#pragma once

#include "system/angband.h"
#include <string>

class CreatureEntity;
class MonsterEntity;
std::string monster_desc(CreatureEntity &subject, const MonsterEntity &monster, BIT_FLAGS mode);
