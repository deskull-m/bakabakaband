/*!
 * @file godot-player-status.cpp
 * @brief プレイヤーステータスのGodot側へのブリッジ実装
 */

#include "main-godot/godot-player-status.h"

#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "system/floor/floor-info.h"
#include "system/gamevalue.h"
#include "system/player-type-definition.h"

#include <mutex>

namespace {

std::mutex g_status_mutex;
GodotPlayerStatusSnapshot g_snapshot;
uint64_t g_version_counter{ 0 };

} // namespace

void player_status_push(const PlayerType *player_ptr)
{
    if (!player_ptr) {
        return;
    }

    GodotPlayerStatusSnapshot snap;
    snap.is_valid = true;
    snap.version = ++g_version_counter;

    snap.name = player_ptr->name;

    if (const auto *race_info = player_ptr->get_race_info(); race_info) {
        snap.race_name = race_info->title.string();
    }
    if (const auto *class_info = player_ptr->get_class_info(); class_info) {
        snap.class_name = class_info->title.string();
    }

    snap.level = player_ptr->get_level();
    const auto *floor = player_ptr->get_floor();
    snap.dun_level = floor ? floor->dun_level : 0;

    snap.chp = player_ptr->get_current_hp();
    snap.mhp = player_ptr->get_max_hp();
    snap.current_mp = static_cast<int>(player_ptr->get_current_mp());
    snap.max_mp = player_ptr->get_max_mp();

    snap.gold = static_cast<long>(player_ptr->get_au());
    snap.exp = static_cast<long>(player_ptr->get_exp());
    snap.max_exp = static_cast<long>(player_ptr->get_max_exp());

    snap.speed = player_ptr->get_speed() - STANDARD_SPEED;
    snap.display_ac = player_ptr->get_dis_ac() + player_ptr->get_dis_to_a();

    for (int i = 0; i < 6; ++i) {
        snap.stat_use[i] = player_ptr->get_stat_use(i);
        snap.stat_top[i] = player_ptr->get_stat_top(i);
    }

    std::lock_guard<std::mutex> lock(g_status_mutex);
    g_snapshot = std::move(snap);
}

GodotPlayerStatusSnapshot player_status_get_snapshot()
{
    std::lock_guard<std::mutex> lock(g_status_mutex);
    return g_snapshot;
}
