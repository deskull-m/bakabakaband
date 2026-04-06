#include "mspell/mspell-attack-util.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"

msa_type::msa_type(CreatureEntity &creature, MONSTER_IDX m_idx)
    : m_idx(m_idx)
    , m_ptr(&creature.current_floor_ptr->get_monster(m_idx))
    , x(creature.x)
    , y(creature.y)
    , do_spell(DO_SPELL_NONE)
    , thrown_spell(MonsterAbilityType::MAX)
{
    this->r_ptr = &this->m_ptr->get_monrace();
    this->no_inate = !evaluate_percent(this->r_ptr->freq_spell * 2);
    this->ability_flags = this->r_ptr->ability_flags;
}

Pos2D msa_type::get_position() const
{
    return Pos2D(this->y, this->x);
}

void msa_type::set_position(const Pos2D &pos)
{
    this->y = pos.y;
    this->x = pos.x;
}

Pos2D msa_type::get_position_lite() const
{
    return Pos2D(this->y_br_lite, this->x_br_lite);
}

void msa_type::set_position_lite(const Pos2D &pos)
{
    this->y_br_lite = pos.y;
    this->x_br_lite = pos.x;
}
