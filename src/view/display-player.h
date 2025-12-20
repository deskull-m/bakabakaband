#pragma once

#include "system/angband.h"
#include <tl/optional.hpp>

class CreatureEntity;
class PlayerType;
tl::optional<int> display_player(CreatureEntity *creature_ptr, const int tmp_mode);
void display_player_equippy(PlayerType *player_ptr, TERM_LEN y, TERM_LEN x, BIT_FLAGS16 mode);
