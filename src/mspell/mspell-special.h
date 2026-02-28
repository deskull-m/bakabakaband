#pragma once

#include "system/angband.h"

struct MonsterSpellResult;

class CreatureEntity;
MonsterSpellResult spell_RF6_SPECIAL(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
