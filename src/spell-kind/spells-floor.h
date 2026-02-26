#pragma once

#include "system/angband.h"

class CreatureEntity;
class PlayerType;
void wiz_lite(PlayerType *player_ptr, bool ninja);
void wiz_dark(PlayerType *player_ptr);
void map_area(CreatureEntity &creature, POSITION range);
bool destroy_area(CreatureEntity &creature, const POSITION y1, const POSITION x1, POSITION r, bool in_generate);
