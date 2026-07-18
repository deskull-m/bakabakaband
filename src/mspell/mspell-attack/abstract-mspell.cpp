#include "mspell/mspell-attack/abstract-mspell.h"
#include "monster/monster-update.h"
#include "mspell/mspell-damage-calculator.h"
#include "mspell/mspell-util.h"
#include "system/creature-entity.h"

AbstractMSpellAttack::AbstractMSpellAttack(CreatureEntity &creature, MONSTER_IDX m_idx, MonsterAbilityType ability, MSpellData data, int target_type, std::function<ProjectResult(POSITION, POSITION, int, AttributeType)> fire)
    : creature_ptr(&creature)
    , m_idx(m_idx)
    , t_idx(0)
    , ability(ability)
    , data(std::move(data))
    , target_type(target_type)
    , fire(std::move(fire))
{
}

AbstractMSpellAttack::AbstractMSpellAttack(CreatureEntity &creature, MONSTER_IDX m_idx, MONSTER_IDX t_idx, MonsterAbilityType ability, MSpellData data, int target_type, std::function<ProjectResult(POSITION, POSITION, int, AttributeType)> fire)
    : creature_ptr(&creature)
    , m_idx(m_idx)
    , t_idx(t_idx)
    , ability(ability)
    , data(std::move(data))
    , target_type(target_type)
    , fire(std::move(fire))
{
}

MonsterSpellResult AbstractMSpellAttack::shoot(POSITION y, POSITION x)
{
    if (!this->data.contain) {
        return MonsterSpellResult::make_invalid();
    }

    this->data.msg.output(*this->creature_ptr, this->m_idx, this->t_idx, this->target_type);

    const auto dam = monspell_damage(*this->creature_ptr, this->ability, this->m_idx, DAM_ROLL);
    const auto proj_res = fire(y, x, dam, data.type);
    if (this->target_type == MONSTER_TO_PLAYER) {
        this->data.drs.execute(*this->creature_ptr, this->m_idx);
    }

    return MonsterSpellResult::make_learnable(proj_res.affected_player, dam);
}
