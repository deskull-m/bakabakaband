#pragma once

#include "system/angband.h"

class CreatureEntity;
bool common_saving_throw_control(CreatureEntity &creature, int pow, const CreatureEntity &target);
bool common_saving_throw_charm(CreatureEntity &creature, int pow, const CreatureEntity &target);
int beam_chance(CreatureEntity &creature);
