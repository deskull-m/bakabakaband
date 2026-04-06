/*!
 * @brief 騎兵のレイシャルパワー処理
 * @date 2020/05/16
 * @author Hourier
 */

#include "mind/mind-cavalry.h"
#include "system/creature-entity.h"
#include "cmd-action/cmd-pet.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/smart-learn-types.h"
#include "pet/pet-fall-off.h"
#include "player/player-skill.h"
#include "system/angband-system.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * 荒馬慣らし
 * @param creature クリーチャーへの参照
 * @return 結果はどうあれ騎乗したらTRUE
 */
bool rodeo(CreatureEntity &creature)
{
    if (creature.riding) {
        msg_print(_("今は乗馬中だ。", "You ARE riding."));
        return false;
    }

    if (!do_cmd_riding(creature, true)) {
        return true;
    }

    auto &monster = creature.current_floor_ptr->get_monster(creature.riding);
    const auto &monrace = monster.get_monrace();
    const auto m_name = monster_desc(creature, monster, 0);
    msg_format(_("%sに乗った。", "You ride on %s."), m_name.data());

    if (monster.is_pet()) {
        return true;
    }

    auto rlev = monrace.level;

    if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
        rlev = rlev * 3 / 2;
    }
    if (rlev > 60) {
        rlev = 60 + (rlev - 60) / 2;
    }
    if ((randint1(creature.skill_exp[PlayerSkillKindType::RIDING] / 120 + creature.level * 2 / 3) > rlev) && one_in_(2) &&
        !creature.current_floor_ptr->inside_arena && !AngbandSystem::get_instance().is_phase_out() && monrace.misc_flags.has_not(MonsterMiscType::GUARDIAN) && monrace.misc_flags.has_not(MonsterMiscType::QUESTOR) &&
        (rlev < creature.level * 3 / 2 + randint0(creature.level / 5))) {
        msg_format(_("%sを手なずけた。", "You tame %s."), m_name.data());
        set_pet(creature, monster);
    } else {
        msg_format(_("%sに振り落とされた！", "You have been thrown off by %s."), m_name.data());
        process_fall_off_horse(creature, 1, true);

        /* 落馬処理に失敗してもとにかく乗馬解除 */
        static_cast<PlayerType &>(creature).ride_monster(0);
    }

    return true;
}
