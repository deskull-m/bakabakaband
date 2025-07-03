#include "world/world-collapsion.h"
#include "market/arena-entry.h"
#include "object-enchant/tr-flags.h"
#include "player/player-status-flags.h"
#include "system/player-type-definition.h"
#include "world/world.h"
#include <algorithm>
#include <cmath>

WorldCollapsion world_collapsion;
WorldCollapsion *wc_ptr = &world_collapsion;

WorldCollapsion::WorldCollapsion()
{
    this->collapse_degree = 0;
}

bool WorldCollapsion::is_blown_away()
{
    return this->collapse_degree >= OVER_COLLAPSION_DEGREE;
}

/*!
 * @brief 時空崩壊度自然進行度計算
 */
void WorldCollapsion::plus_timed_world_collapsion(AngbandWorld *w_ptr, PlayerType *player_ptr, int multi)
{
    auto &entries = ArenaEntryList::get_instance();
    if (w_ptr->total_winner && entries.get_current_entry() >= entries.get_max_entries()) {
        return;
    }
    if (get_player_flags(player_ptr, TR_WORLD_END)) {
        multi *= 2;
    }
    if (w_ptr->total_winner) {
        multi /= 3;
    }
    this->collapse_degree += std::min(1, static_cast<int>(std::round(std::sqrt(w_ptr->game_turn / 2000) * multi)));
}

/*!
 * @brief 時空崩壊度に伴う深層モンスター加算処理
 */
DEPTH WorldCollapsion::plus_monster_level()
{
    int pow = this->collapse_degree / OVER_COLLAPSION_DEGREE / 2; // 0-50
    return pow > randint1(100) ? randint1(pow) : 0;
}

/*!
 * @brief 時空崩壊度単純増減
 */
void WorldCollapsion::plus_collapsion(int value)
{
    this->collapse_degree += value;
}

/*!
 * @brief 時空崩壊度比率増減(万分率変動)
 */
void WorldCollapsion::plus_perm_collapsion(int permyriad)
{
    if (permyriad > 0) {
        this->collapse_degree += static_cast<int32_t>((OVER_COLLAPSION_DEGREE - this->collapse_degree) * permyriad / 10000);
    } else {
        this->collapse_degree -= static_cast<int32_t>(this->collapse_degree * permyriad / 10000);
    }
}
