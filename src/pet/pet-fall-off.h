#pragma once

#include "system/angband.h"

class MonsterAttackPlayer;
class CreatureEntity;
void check_fall_off_horse(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr);
bool process_fall_off_horse(CreatureEntity &creature, int dam, bool force);
