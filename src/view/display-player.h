#pragma once

#include "system/angband.h"
#include <tl/optional.hpp>

class CreatureEntity;
tl::optional<int> display_player(CreatureEntity &creature, const int tmp_mode);
void display_player_equippy(CreatureEntity &creature, TERM_LEN y, TERM_LEN x, BIT_FLAGS16 mode);
