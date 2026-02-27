#pragma once

#include "system/angband.h"

class CreatureEntity;
class PlayerType;
bool polymorph_monster(CreatureEntity &creature, POSITION y, POSITION x);
bool trans_sex(PlayerType *player_ptr);
