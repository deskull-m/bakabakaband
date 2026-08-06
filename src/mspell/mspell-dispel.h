#pragma once

#include "system/angband.h"

struct MonsterSpellResult;

class CreatureEntity;
MonsterSpellResult spell_RF4_DISPEL(MONSTER_IDX m_idx, CreatureEntity &creature, MONSTER_IDX t_idx, int target_type);
