#pragma once

#include "system/angband.h"

class CreatureEntity;
class MonsterEntity;
bool common_saving_throw_control(CreatureEntity &creature, int pow, const MonsterEntity &monster);
bool common_saving_throw_charm(CreatureEntity &creature, int pow, const MonsterEntity &monster);
int beam_chance(CreatureEntity &creature);
