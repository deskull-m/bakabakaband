#pragma once

#include "system/angband.h"

class CreatureEntity;
class PlayerType;
bool set_ele_attack(CreatureEntity &creature, uint32_t attack_type, TIME_EFFECT v);
bool set_ele_immune(CreatureEntity &creature, uint32_t immune_type, TIME_EFFECT v);
bool choose_ele_attack(CreatureEntity &creature, TIME_EFFECT turn);
bool choose_ele_immune(CreatureEntity &creature, TIME_EFFECT turn);
bool pulish_shield(CreatureEntity &creature);
