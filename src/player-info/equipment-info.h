#pragma once

#include "system/angband.h"

class CreatureEntity;
bool has_melee_weapon(CreatureEntity &creature, int i);
BIT_FLAGS16 empty_hands(CreatureEntity &creature, bool riding_control);
bool can_two_hands_wielding(CreatureEntity &creature);
bool heavy_armor(CreatureEntity &creature);
