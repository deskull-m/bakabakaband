#pragma once

#include "system/angband.h"

#include "effect/attribute-types.h"
#include "mspell/mspell-attack/abstract-mspell.h"

struct MonsterSpellResult;

class CreatureEntity;
MonsterSpellResult spell_RF4_BREATH(CreatureEntity &creature, MonsterAbilityType ms_type, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
