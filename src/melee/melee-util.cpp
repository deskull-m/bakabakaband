#include "melee/melee-util.h"
#include "floor/geometry.h"
#include "grid/grid.h"
#include "melee/melee-switcher.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "timed-effect/timed-effects.h"

mam_type *initialize_mam_type(CreatureEntity &creature, mam_type *mam_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx)
{
    mam_ptr->attribute = BlowEffectType::NONE;
    mam_ptr->m_idx = m_idx;
    mam_ptr->t_idx = t_idx;
    mam_ptr->m_ptr = &creature.get_floor()->get_monster(m_idx);
    mam_ptr->t_ptr = &creature.get_floor()->get_monster(t_idx);
    mam_ptr->damage = 0;
    if (creature.is_player()) {
        mam_ptr->see_m = is_seen(creature, *mam_ptr->m_ptr);
        mam_ptr->see_t = is_seen(creature, *mam_ptr->t_ptr);
        mam_ptr->do_silly_attack = one_in_(2) && creature.is_hallucinated();
    } else {
        mam_ptr->see_m = false;
        mam_ptr->see_t = false;
        mam_ptr->do_silly_attack = false;
    }
    mam_ptr->see_either = mam_ptr->see_m || mam_ptr->see_t;
    mam_ptr->y_saver = mam_ptr->t_ptr->y;
    mam_ptr->x_saver = mam_ptr->t_ptr->x;
    mam_ptr->explode = false;
    mam_ptr->touched = false;

    const auto &monrace = mam_ptr->m_ptr->get_monrace();
    mam_ptr->ac = static_cast<ARMOUR_CLASS>(mam_ptr->t_ptr->get_ac());
    mam_ptr->rlev = ((monrace.level >= 1) ? monrace.level : 1);
    mam_ptr->blinked = false;
    mam_ptr->power = 0;
    mam_ptr->obvious = false;
    const auto p_pos = creature.get_position();
    mam_ptr->known = (Grid::calc_distance(p_pos, mam_ptr->m_ptr->get_position()) <= MAX_PLAYER_SIGHT) || (Grid::calc_distance(p_pos, mam_ptr->t_ptr->get_position()) <= MAX_PLAYER_SIGHT);
    mam_ptr->fear = false;
    mam_ptr->dead = false;
    return mam_ptr;
}
