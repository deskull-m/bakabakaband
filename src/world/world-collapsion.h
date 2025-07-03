#pragma once
#include "system/h-type.h"
#include <cstdint>

class AngbandWorld;
class PlayerType;

class WorldCollapsion {
private:
    const int32_t OVER_COLLAPSION_DEGREE = 100000000LL;

public:
    WorldCollapsion();
    void plus_timed_world_collapsion(AngbandWorld *w_ptr, PlayerType *player_ptr, int multi);
    void plus_collapsion(int value);
    void plus_perm_collapsion(int permyriad);
    DEPTH plus_monster_level();
    bool is_blown_away();

    int32_t collapse_degree{}; /*!< 時空崩壊度 */
    WorldCollapsion &operator=(const WorldCollapsion &) = delete;
};

extern WorldCollapsion world_collapsion;
extern WorldCollapsion *wc_ptr;
