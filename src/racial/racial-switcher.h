#pragma once

#include "system/angband.h"

class CreatureEntity;
bool switch_class_racial_execution(CreatureEntity &creature, const int32_t command);
bool switch_mimic_racial_execution(CreatureEntity &creature);
bool switch_race_racial_execution(CreatureEntity &creature, const int32_t command);
