/*!
 * @file godot-world-snapshot.cpp
 * @brief 観測可能ワールドモデルの Godot 側へのブリッジ実装
 */

#include "main-godot/godot-world-snapshot.h"

#include "system/player-type-definition.h"
#include "view/observable-world.h"
#include "world/world.h"

#include <atomic>
#include <mutex>

namespace {

std::atomic<bool> g_enabled{ false };
std::atomic<uint64_t> g_version{ 0 };
std::mutex g_mutex;
GodotWorldSnapshot g_snapshot;

} // namespace

void world_snapshot_set_enabled(bool enabled)
{
    g_enabled.store(enabled);
}

bool world_snapshot_is_enabled()
{
    return g_enabled.load();
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

    auto world = std::make_shared<const ObservableWorld>(build_observable_world(*player_ptr));
    if (!world->is_valid) {
        return;
    }

    std::lock_guard lock(g_mutex);
    g_snapshot.version = g_version.load() + 1;
    g_snapshot.world = std::move(world);
    g_version.store(g_snapshot.version);
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
