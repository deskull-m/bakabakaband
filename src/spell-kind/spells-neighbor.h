#pragma once

#include "system/angband.h"

class CreatureEntity;
bool door_creation(CreatureEntity &creature, POSITION y, POSITION x);
bool trap_creation(CreatureEntity &creature, POSITION y, POSITION x);
bool tree_creation(CreatureEntity &creature, POSITION y, POSITION x);
bool wall_creation(CreatureEntity &creature, POSITION y, POSITION x);
bool create_rune_protection_area(CreatureEntity &creature, POSITION y, POSITION x);
bool wall_stone(CreatureEntity &creature);
bool destroy_doors_touch(CreatureEntity &creature);
bool disarm_traps_touch(CreatureEntity &creature);
bool sleep_monsters_touch(CreatureEntity &creature);
bool animate_dead(CreatureEntity &creature, MONSTER_IDX src_idx, POSITION y, POSITION x);
void wall_breaker(CreatureEntity &creature);
