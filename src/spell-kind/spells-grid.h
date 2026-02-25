#pragma once

#include "system/angband.h"

class CreatureEntity;
class PlayerType;
bool create_rune_protection_one(CreatureEntity &creature);
bool create_rune_explosion(PlayerType *player_ptr, POSITION y, POSITION x);
void stair_creation(PlayerType *player_ptr);
