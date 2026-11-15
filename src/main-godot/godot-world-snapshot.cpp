/*!
 * @file godot-world-snapshot.cpp
 * @brief 観測可能ワールドモデルの Godot 側へのブリッジ実装
 */

#include "main-godot/godot-world-snapshot.h"

#include "system/player-type-definition.h"
#include "view/map-info-cache.h"
#include "view/observable-world.h"
#include "view/view-observer.h"
#include "world/world.h"

#include <godot_cpp/variant/utility_functions.hpp>

#include <atomic>
#include <mutex>

namespace {

std::atomic<bool> g_enabled{ false };
std::atomic<bool> g_verify{ false };
std::atomic<uint64_t> g_verify_mismatches{ 0 };
std::atomic<uint64_t> g_version{ 0 };
std::mutex g_mutex;
GodotWorldSnapshot g_snapshot; ///< g_mutex で保護。Godot メインスレッドへ渡す最新のもの

/*!
 * @brief 変わったかもしれないグリッドを集める観測者
 * @details 通知も push もゲームスレッドから呼ばれるので、ロックは要らない
 */
class DirtyGridCollector : public ViewObserver {
public:
    void on_grid_changed(const Pos2D &pos) override
    {
        if (!g_enabled.load(std::memory_order_relaxed)) {
            // 無効の間の変化は追わない。次に有効になったときはフロア全体を作り直す
            this->all_dirty = true;
            return;
        }

        if (this->all_dirty) {
            return;
        }

        // 1 回の push までに通知が溜まりすぎたら、個別に求め直すよりフロア全体を作り直す方が速い
        if (this->dirty.size() >= MAX_DIRTY_GRIDS) {
            this->on_all_grids_changed();
            return;
        }

        this->dirty.push_back(pos);
    }

    void on_all_grids_changed() override
    {
        this->all_dirty = true;
        this->dirty.clear();
    }

    static constexpr size_t MAX_DIRTY_GRIDS = 8192;

    bool all_dirty = true;
    std::vector<Pos2D> dirty;
};

DirtyGridCollector g_collector;

/// ゲームスレッド専用: 前回 push した結果 (差分の基準)
GodotWorldSnapshot g_last;

bool is_same_floor(const ObservableWorld &a, const ObservableWorld &b)
{
    return (a.width == b.width) && (a.height == b.height) && (a.dungeon_id == b.dungeon_id) && (a.dun_level == b.dun_level) && (a.is_wild_mode == b.is_wild_mode);
}

void verify_against_full_build(PlayerType &player, const ObservableWorld &world)
{
    const auto full = build_observable_world(player);
    if (full.grids.size() != world.grids.size()) {
        g_verify_mismatches.fetch_add(1);
        godot::UtilityFunctions::printerr("World snapshot verify: grid count mismatch");
        return;
    }

    uint64_t mismatches = 0;
    for (size_t i = 0; i < full.grids.size(); ++i) {
        if (full.grids[i] == world.grids[i]) {
            continue;
        }

        if (mismatches == 0) {
            const auto x = static_cast<int>(i) % world.width;
            const auto y = static_cast<int>(i) / world.width;
            godot::UtilityFunctions::printerr("World snapshot verify: stale grid at (", x, ", ", y, ")");
        }
        ++mismatches;
    }

    if (mismatches > 0) {
        g_verify_mismatches.fetch_add(mismatches);
        godot::UtilityFunctions::printerr("World snapshot verify: ", static_cast<int64_t>(mismatches), " stale grid(s)");
    }
}

} // namespace

void world_snapshot_install_observer()
{
    set_view_observer(&g_collector);
}

void world_snapshot_set_enabled(bool enabled)
{
    g_enabled.store(enabled);
}

bool world_snapshot_is_enabled()
{
    return g_enabled.load();
}

void world_snapshot_set_verify(bool verify)
{
    g_verify.store(verify);
}

uint64_t world_snapshot_get_verify_mismatches()
{
    return g_verify_mismatches.load();
}

void world_snapshot_push(PlayerType *player_ptr)
{
    if (!g_enabled.load() || !player_ptr) {
        return;
    }

    // キャラクター作成中・フロア生成前はフロアの中身が定まっていない
    if (!AngbandWorld::get_instance().character_dungeon) {
        return;
    }

    auto &player = *player_ptr;
    auto world = std::make_shared<ObservableWorld>(build_observable_world_header(player));
    if (!world->is_valid) {
        return;
    }

    const auto version = g_version.load() + 1;
    const auto grid_count = static_cast<size_t>(world->width * world->height);
    std::shared_ptr<std::vector<uint64_t>> grid_versions;
    auto floor_version = g_last.floor_version;
    const auto can_diff = !g_collector.all_dirty && g_last.world && is_same_floor(*g_last.world, *world);
    if (can_diff) {
        world->grids = g_last.world->grids;
        grid_versions = std::make_shared<std::vector<uint64_t>>(*g_last.grid_versions);
        MapInfoCache::get_instance().prepare(player);
        for (const auto &pos : g_collector.dirty) {
            if ((pos.y < 0) || (pos.y >= world->height) || (pos.x < 0) || (pos.x >= world->width)) {
                continue;
            }

            const auto index = static_cast<size_t>(pos.y * world->width + pos.x);
            auto observed = observe_grid(player, pos);
            if (observed != world->grids[index]) {
                world->grids[index] = observed;
                (*grid_versions)[index] = version;
            }
        }
    } else {
        auto full = build_observable_world(player);
        world->grids = std::move(full.grids);
        grid_versions = std::make_shared<std::vector<uint64_t>>(grid_count, version);
        floor_version = version;
    }

    world->creatures = observe_creatures(player);
    g_collector.all_dirty = false;
    g_collector.dirty.clear();

    if (can_diff && g_verify.load()) {
        verify_against_full_build(player, *world);
    }

    GodotWorldSnapshot snapshot{ version, floor_version, std::move(world), std::move(grid_versions) };
    g_last = snapshot;
    std::lock_guard lock(g_mutex);
    g_snapshot = std::move(snapshot);
    g_version.store(version);
}

uint64_t world_snapshot_get_version()
{
    return g_version.load();
}

GodotWorldSnapshot world_snapshot_get()
{
    std::lock_guard lock(g_mutex);
    return g_snapshot;
}
