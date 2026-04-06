#include "effect/effect-player-spirit.h"
#include "blue-magic/blue-magic-checker.h"
#include "core/window-redrawer.h"
#include "effect/effect-player.h"
#include "mind/mind-mirror-master.h"
#include "monster/monster-util.h"
#include "player/player-damage.h"
#include "player/player-status-flags.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "system/creature-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "tracking/health-bar-tracker.h"
#include "view/display-messages.h"
#include "world/world.h"

void effect_player_drain_mana(CreatureEntity &creature, EffectPlayerType *ep_ptr)
{
    if (check_multishadow(creature)) {
        msg_print(_("攻撃は幻影に命中し、あなたには届かなかった。", "The attack hits Shadow, but you are unharmed!"));
        ep_ptr->dam = 0;
        return;
    }

    if (creature.csp == 0) {
        ep_ptr->dam = 0;
        return;
    }

    if (ep_ptr->is_monster()) {
        msg_format(_("%s^に精神エネルギーを吸い取られてしまった！", "%s^ draws psychic energy from you!"), ep_ptr->m_name.data());
    } else {
        msg_print(_("精神エネルギーを吸い取られてしまった！", "Your psychic energy is drained!"));
    }

    if (ep_ptr->dam >= creature.csp) {
        ep_ptr->dam = creature.csp;
        creature.csp = 0;
        creature.csp_frac = 0;
    } else {
        creature.csp -= ep_ptr->dam;
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::MP);
    static constexpr auto flags = {
        SubWindowRedrawingFlag::PLAYER,
        SubWindowRedrawingFlag::SPELL,
    };
    rfu.set_flags(flags);

    if (!ep_ptr->is_monster() || (ep_ptr->m_ptr->hp >= ep_ptr->m_ptr->maxhp)) {
        ep_ptr->dam = 0;
        return;
    }

    ep_ptr->m_ptr->hp += ep_ptr->dam;
    if (ep_ptr->m_ptr->hp > ep_ptr->m_ptr->maxhp) {
        ep_ptr->m_ptr->hp = ep_ptr->m_ptr->maxhp;
    }

    HealthBarTracker::get_instance().set_flag_if_tracking(ep_ptr->src_idx);
    if (ep_ptr->src_ptr && ep_ptr->src_ptr->is_riding()) {
        rfu.set_flag(MainWindowRedrawingFlag::UHEALTH);
    }

    if (ep_ptr->m_ptr->get_monster_profile().ml) {
        msg_format(_("%s^は気分が良さそうだ。", "%s^ appears healthier."), ep_ptr->m_name.data());
    }

    ep_ptr->dam = 0;
}

void effect_player_mind_blast(CreatureEntity &creature, EffectPlayerType *ep_ptr)
{
    if ((randint0(100 + ep_ptr->rlev / 2) < std::max<short>(5, creature.skill_sav)) && !check_multishadow(creature)) {
        msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
        return;
    }

    if (check_multishadow(creature)) {
        ep_ptr->get_damage = take_hit(creature, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
        return;
    }

    msg_print(_("霊的エネルギーで精神が攻撃された。", "Your mind is blasted by psionic energy."));
    BadStatusSetter bss(creature);
    if (!has_resist_conf(creature)) {
        (void)bss.mod_confusion(randint0(4) + 4);
    }

    if (!has_resist_chaos(creature) && one_in_(3)) {
        (void)bss.mod_hallucination(randint0(250) + 150);
    }

    creature.csp -= 50;
    if (creature.csp < 0) {
        creature.csp = 0;
        creature.csp_frac = 0;
    }

    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::MP);
    ep_ptr->get_damage = take_hit(creature, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_brain_smash(CreatureEntity &creature, EffectPlayerType *ep_ptr)
{
    if ((randint0(100 + ep_ptr->rlev / 2) < std::max<short>(5, creature.skill_sav)) && !check_multishadow(creature)) {
        msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
        return;
    }

    if (!check_multishadow(creature)) {
        msg_print(_("霊的エネルギーで精神が攻撃された。", "Your mind is blasted by psionic energy."));
        creature.csp -= 100;
        if (creature.csp < 0) {
            creature.csp = 0;
            creature.csp_frac = 0;
        }

        RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::MP);
    }

    ep_ptr->get_damage = take_hit(creature, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
    if (check_multishadow(creature)) {
        return;
    }

    BadStatusSetter bss(creature);
    if (!has_resist_blind(creature)) {
        (void)bss.mod_blindness(8 + randint0(8));
    }

    if (!has_resist_conf(creature)) {
        (void)bss.mod_confusion(randint0(4) + 4);
    }

    if (!creature.free_act) {
        (void)bss.mod_paralysis(randint0(4) + 4);
    }

    (void)bss.mod_deceleration(randint0(4) + 4, false);

    while (randint0(100 + ep_ptr->rlev / 2) > (std::max<short>(5, creature.skill_sav))) {
        (void)do_dec_stat(creature, A_INT);
    }
    while (randint0(100 + ep_ptr->rlev / 2) > (std::max<short>(5, creature.skill_sav))) {
        (void)do_dec_stat(creature, A_WIS);
    }

    if (!has_resist_chaos(creature)) {
        (void)bss.mod_hallucination(randint0(250) + 150);
    }
}
