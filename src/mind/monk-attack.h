#pragma once
#include "system/h-type.h"

struct player_attack_type;
class CreatureEntity;
void process_monk_attack(CreatureEntity &creature, player_attack_type *pa_ptr);
bool double_attack(CreatureEntity &creature);
WEIGHT calc_monk_attack_weight(CreatureEntity &creature);
