/*!
 * @file godot-world-snapshot.cpp
 * @brief 観測可能ワールドモデルの Godot 側へのブリッジ実装
 */

#include "main-godot/godot-world-snapshot.h"
#include "main-godot/godot-game-events.h"

#include "system/player-type-definition.h"
#include "view/map-info-cache.h"
#include "view/observable-world.h"
#include "view/view-observer.h"
#include "world/world.h"

#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <mutex>
#include <unordered_map>

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

ViewEvent make_move_event(ViewEventType type, uint32_t id, bool is_player, const Pos2D &from, const Pos2D &to)
{
    ViewEvent event;
    event.type = type;
    event.actor.id = id;
    event.actor.is_player = is_player;
    event.actor.pos = to;
    event.actor.has_pos = type != ViewEventType::DISAPPEAR;
    event.target.pos = from;
    event.target.has_pos = type != ViewEventType::APPEAR;
    return event;
}

ViewEventType classify_move(const Pos2D &from, const Pos2D &to)
{
    const auto distance = std::max(std::abs(to.y - from.y), std::abs(to.x - from.x));
    return distance <= 1 ? ViewEventType::MOVE : ViewEventType::TELEPORT;
}

/*!
 * @brief 前回と今回のスナップショットの差から移動のイベントを作る
 * @details MOVE / TELEPORT は actor.pos が移動先、target.pos が移動元。
 * 同じ階の中だけで作る (階を移ったら Godot 側は全体を作り直すので不要)
 */
void push_move_events(const ObservableWorld &previous, const ObservableWorld &current, uint32_t player_id)
{
    if (previous.player_pos != current.player_pos) {
        game_events_push(make_move_event(classify_move(previous.player_pos, current.player_pos), player_id, true, previous.player_pos, current.player_pos));
    }

    std::unordered_map<uint32_t, Pos2D> previous_positions;
    for (const auto &creature : previous.creatures) {
        previous_positions.emplace(creature.display_id, creature.pos);
    }

    for (const auto &creature : current.creatures) {
        const auto it = previous_positions.find(creature.display_id);
        if (it == previous_positions.end()) {
            game_events_push(make_move_event(ViewEventType::APPEAR, creature.display_id, false, creature.pos, creature.pos));
            continue;
        }

        if (it->second != creature.pos) {
            game_events_push(make_move_event(classify_move(it->second, creature.pos), creature.display_id, false, it->second, creature.pos));
        }
        previous_positions.erase(it);
    }

    for (const auto &[id, pos] : previous_positions) {
        game_events_push(make_move_event(ViewEventType::DISAPPEAR, id, false, pos, pos));
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
    // グリッドを全て作り直した回でも、同じフロアなら移動は起きている (別のフロアへの移動は移動として扱わない)
    if (g_last.world && is_same_floor(*g_last.world, *world) && game_events_is_enabled()) {
        push_move_events(*g_last.world, *world, player.get_display_id());
    }
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
