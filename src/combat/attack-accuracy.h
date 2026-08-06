#pragma once

#include "system/angband.h"

struct player_attack_type;
class CreatureEntity;
bool test_hit_norm(CreatureEntity &creature, int chance, ARMOUR_CLASS ac, bool visible);
PERCENTAGE hit_chance(CreatureEntity &creature, int chance, ARMOUR_CLASS ac);
bool check_hit_from_monster_to_player(CreatureEntity &creature, int power, DEPTH level, int stun);
bool check_hit_from_monster_to_monster(int power, DEPTH level, ARMOUR_CLASS ac, int stun);
bool process_attack_hit(CreatureEntity &creature, player_attack_type *pa_ptr, int chance);
