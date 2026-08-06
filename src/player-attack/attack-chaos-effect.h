#pragma once

#include "system/angband.h"

struct player_attack_type;
class CreatureEntity;
void change_monster_stat(CreatureEntity &creature, player_attack_type *pa_ptr, const POSITION y, const POSITION x, int *num);
