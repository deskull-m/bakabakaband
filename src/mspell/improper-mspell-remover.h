#pragma once

#include "system/angband.h"

#include "monster-race/race-ability-flags.h"
#include "util/flag-group.h"

class CreatureEntity;
void remove_bad_spells(MONSTER_IDX m_idx, CreatureEntity &creature, EnumClassFlagGroup<MonsterAbilityType> &ability_flags);
