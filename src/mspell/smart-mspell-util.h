#pragma once

#include "monster-race/race-ability-flags.h"
#include "monster/smart-learn-types.h"
#include "util/flag-group.h"

// Monster Spell Remover.
class CreatureEntity;
class MonraceDefinition;
struct msr_type {
    msr_type(CreatureEntity &creature, short m_idx, const EnumClassFlagGroup<MonsterAbilityType> &ability_flags);
    MonraceDefinition *r_ptr;
    EnumClassFlagGroup<MonsterAbilityType> ability_flags;
    EnumClassFlagGroup<MonsterSmartLearnType> smart_flags{};
};

bool int_outof(const MonraceDefinition &monrace, int prob);
