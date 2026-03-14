#pragma once

#include "system/angband.h"

#include "effect/attribute-types.h"
#include "monster-race/race-ability-flags.h"
#include "mspell/mspell-data.h"
#include "mspell/mspell-util.h"
#include "system/creature-entity.h"
#include <type_traits>

struct MonsterSpellResult;

class CurseData : public MSpellData {
public:
    CurseData(const std::string_view &msg1, const std::string_view &msg2, const std::string_view &msg3, const AttributeType &typ);
    template <typename FUNC>
    CurseData(FUNC msg_func, AttributeType typ)
        : MSpellData(msg_func, typ)
    {
        static_assert(std::is_invocable<decltype(msg_func), CreatureEntity &, MONSTER_IDX, MONSTER_IDX, int>::value);
    }
};

class CreatureEntity;
MonsterSpellResult spell_RF5_CAUSE(CreatureEntity &creature, MonsterAbilityType ms_type, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
