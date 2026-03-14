#pragma once

#include "mspell/mspell-attack/abstract-mspell.h"
#include "system/angband.h"

struct MonsterSpellResult;
class MSpellData;

class MSpellAttackOther : public AbstractMSpellAttack {
public:
    MSpellAttackOther(CreatureEntity &creature, MONSTER_IDX m_idx, MonsterAbilityType ability, MSpellData data, int target_type, std::function<ProjectResult(POSITION, POSITION, int, AttributeType)> fire);
    MSpellAttackOther(CreatureEntity &creature, MONSTER_IDX m_idx, MONSTER_IDX t_idx, MonsterAbilityType ability, MSpellData data, int target_type, std::function<ProjectResult(POSITION, POSITION, int, AttributeType)> fire);
    ~MSpellAttackOther() = default;
    MSpellAttackOther(const MSpellAttackOther &) = delete;
    MSpellAttackOther(MSpellAttackOther &&) = default;
    void operator=(const MSpellAttackOther &) = delete;
    MSpellAttackOther &operator=(MSpellAttackOther &&) = default;
};
MonsterSpellResult spell_RF4_ROCKET(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_HAND_DOOM(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_PSY_SPEAR(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
