#include "effect/spells-effect-util.h"
#include "monster/monster-describer.h"
#include "pet/pet-fall-off.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "view/display-messages.h"
#include <algorithm>

bool sukekaku;

int project_length = 0;

int project_m_n;
POSITION project_m_x;
POSITION project_m_y;
POSITION monster_target_x;
POSITION monster_target_y;

CapturedMonsterType::CapturedMonsterType()
    : r_idx(MonraceId::PLAYER)
{
    mflag2.clear();
}

FallOffHorseEffect::FallOffHorseEffect(CreatureEntity &creature)
    : creature_ptr(&creature)
{
}

void FallOffHorseEffect::set_shake_off(int damage)
{
    this->shake_off_damage = std::min(damage, FALL_OFF_DAMAGE_MAX);
}

void FallOffHorseEffect::set_fall_off(int damage)
{
    this->fall_off_damage = std::min(damage, FALL_OFF_DAMAGE_MAX);
}

void FallOffHorseEffect::apply() const
{
    if (!this->creature_ptr->riding) {
        return;
    }

    if (this->shake_off_damage <= 0 && this->fall_off_damage <= 0) {
        return;
    }

    const auto &floor = *this->creature_ptr->get_floor();
    const auto m_name = monster_desc(*this->creature_ptr, floor.get_monster(creature_ptr->riding), 0);

    if (this->shake_off_damage > 0) {
        if (process_fall_off_horse(*this->creature_ptr, this->shake_off_damage, false)) {
            msg_format(_("%s^に振り落とされた！", "%s^ has thrown you off!"), m_name.data());
            return;
        }
    }

    if (this->fall_off_damage > 0) {
        if (process_fall_off_horse(*this->creature_ptr, this->fall_off_damage, false)) {
            msg_format(_("%s^から落ちてしまった！", "You have fallen from %s."), m_name.data());
        }
    }
}
