#include "spell/spells-staff-only.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "hpmp/hp-mp-processor.h"
#include "player-base/player-class.h"
#include "player/player-damage.h"
#include "spell-kind/spells-sight.h"
#include "status/bad-status-setter.h"
#include "status/body-improvement.h"
#include "system/creature-entity.h"
#include "view/display-messages.h"

/*!
 * @brief 聖浄の杖の効果
 * @param creature クリーチャーへの参照
 * @magic 魔法の効果である場合TRUE (杖と同じ効果の呪文はあったか？ 要調査)
 * @powerful 効果が増強される時TRUE (TRUEになるタイミングはあるか？ 要調査)
 */
bool cleansing_nova(CreatureEntity &creature, bool magic, bool powerful)
{
    auto ident = dispel_evil(creature, powerful ? 225 : 150);
    const auto k = 3 * creature.get_level();
    const short turns = randint1(25) + k;
    BodyImprovement improvement(creature);
    if (magic) {
        improvement.set_protection(turns);
    } else {
        improvement.mod_protection(turns);
    }

    if (improvement.has_effect()) {
        ident = true;
    }

    BadStatusSetter bss(creature);
    if (bss.set_poison(0)) {
        ident = true;
    }

    if (bss.set_fear(0)) {
        ident = true;
    }

    if (hp_player(creature, 50)) {
        ident = true;
    }

    if (bss.set_stun(0)) {
        ident = true;
    }

    if (bss.set_cut(0)) {
        ident = true;
    }

    return ident;
}

/*!
 * @brief 魔力の嵐の杖の効果
 * @param creature クリーチャーへの参照
 * @powerful 効果が増強される時TRUE (TRUEになるタイミングはあるか？ 要調査)
 */
bool unleash_mana_storm(CreatureEntity &creature, bool powerful)
{
    msg_print(_("強力な魔力が敵を引き裂いた！", "Mighty magics rend your enemies!"));
    project(creature, 0, (powerful ? 7 : 5), creature.y, creature.x, (randint1(200) + (powerful ? 500 : 300)) * 2, AttributeType::MANA,
        PROJECT_KILL | PROJECT_ITEM | PROJECT_GRID);

    if (!CreatureClass(creature).is_wizard()) {
        (void)take_hit(creature, DAMAGE_NOESCAPE, 50, _("コントロールし難い強力な魔力の解放", "unleashing magics too mighty to control"));
    }

    return true;
}
