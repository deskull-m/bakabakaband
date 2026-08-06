#pragma once

#include "system/angband.h"
#include <string>

class CreatureEntity;
std::string monster_desc(CreatureEntity &subject, const CreatureEntity &monster, BIT_FLAGS mode);
