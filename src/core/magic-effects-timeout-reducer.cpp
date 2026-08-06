#include "core/magic-effects-timeout-reducer.h"
#include "game-option/birth-options.h"
#include "mind/mind-force-trainer.h"
#include "mind/mind-magic-resistance.h"
#include "mind/mind-mirror-master.h"
#include "player/player-status-table.h"
#include "racial/racial-kutar.h"
#include "spell-realm/spells-craft.h"
#include "spell-realm/spells-crusade.h"
#include "spell-realm/spells-demon.h"
#include "spell-realm/spells-song.h"
#include "status/bad-status-setter.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "status/shape-changer.h"
#include "status/sight-setter.h"
#include "status/temporary-resistance.h"
#include "system/creature-entity.h"
#include "timed-effect/player-cut.h"

/*!
 * @brief 10ゲームターンが進行するごとに魔法効果の残りターンを減らしていく処理
 * / Handle timeout every 10 game turns
 */
void reduce_magic_effects_timeout(CreatureEntity &creature)
{
    if (creature.get_timed_effect(CreatureTimedEffect::TIM_MIMIC)) {
        (void)set_mimic(creature, creature.get_timed_effect(CreatureTimedEffect::TIM_MIMIC) - 1, creature.get_mimic_form(), true);
    }

    BadStatusSetter bss(creature);
    if (creature.is_hallucinated()) {
        (void)bss.mod_hallucination(-1);
    }

    if (creature.is_blind()) {
        (void)bss.mod_blindness(-1);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::TIM_INVIS)) {
        (void)set_tim_invis(creature, creature.get_timed_effect(CreatureTimedEffect::TIM_INVIS) - 1, true);
    }

    if (creature.is_suppress_multi_reward()) {
        creature.set_suppress_multi_reward(false);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::TIM_ESP)) {
        (void)set_tim_esp(creature, creature.get_timed_effect(CreatureTimedEffect::TIM_ESP) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::ELE_ATTACK)) {
        creature.set_timed_effect(CreatureTimedEffect::ELE_ATTACK, creature.get_timed_effect(CreatureTimedEffect::ELE_ATTACK) - 1);
        if (!creature.get_timed_effect(CreatureTimedEffect::ELE_ATTACK)) {
            set_ele_attack(creature, 0, 0);
        }
    }

    if (creature.get_timed_effect(CreatureTimedEffect::ELE_IMMUNE)) {
        creature.set_timed_effect(CreatureTimedEffect::ELE_IMMUNE, creature.get_timed_effect(CreatureTimedEffect::ELE_IMMUNE) - 1);
        if (!creature.get_timed_effect(CreatureTimedEffect::ELE_IMMUNE)) {
            set_ele_immune(creature, 0, 0);
        }
    }

    if (creature.get_timed_effect(CreatureTimedEffect::TIM_INFRA)) {
        (void)set_tim_infra(creature, creature.get_timed_effect(CreatureTimedEffect::TIM_INFRA) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::TIM_STEALTH)) {
        (void)set_tim_stealth(creature, creature.get_timed_effect(CreatureTimedEffect::TIM_STEALTH) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::TIM_LEVITATION)) {
        (void)set_tim_levitation(creature, creature.get_timed_effect(CreatureTimedEffect::TIM_LEVITATION) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::TIM_SH_TOUKI)) {
        (void)set_tim_sh_force(creature, creature.get_timed_effect(CreatureTimedEffect::TIM_SH_TOUKI) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::TIM_SH_FIRE)) {
        (void)set_tim_sh_fire(creature, creature.get_timed_effect(CreatureTimedEffect::TIM_SH_FIRE) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::TIM_SH_HOLY)) {
        (void)set_tim_sh_holy(creature, creature.get_timed_effect(CreatureTimedEffect::TIM_SH_HOLY) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::TIM_EYEEYE)) {
        (void)set_tim_eyeeye(creature, creature.get_timed_effect(CreatureTimedEffect::TIM_EYEEYE) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::RESIST_MAGIC)) {
        (void)set_resist_magic(creature, creature.get_timed_effect(CreatureTimedEffect::RESIST_MAGIC) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::TIM_REGEN)) {
        (void)set_tim_regen(creature, creature.get_timed_effect(CreatureTimedEffect::TIM_REGEN) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::TIM_RES_NETHER)) {
        (void)set_tim_res_nether(creature, creature.get_timed_effect(CreatureTimedEffect::TIM_RES_NETHER) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::TIM_RES_LITE)) {
        (void)set_tim_res_lite(creature, creature.get_timed_effect(CreatureTimedEffect::TIM_RES_LITE) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::TIM_RES_DARK)) {
        (void)set_tim_res_dark(creature, creature.get_timed_effect(CreatureTimedEffect::TIM_RES_DARK) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::TIM_RES_FEAR)) {
        (void)set_tim_res_fear(creature, creature.get_timed_effect(CreatureTimedEffect::TIM_RES_FEAR) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::TIM_RES_TIME)) {
        (void)set_tim_res_time(creature, creature.get_timed_effect(CreatureTimedEffect::TIM_RES_TIME) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::TIM_REFLECT)) {
        (void)set_tim_reflect(creature, creature.get_timed_effect(CreatureTimedEffect::TIM_REFLECT) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::MULTISHADOW)) {
        (void)set_multishadow(creature, creature.get_timed_effect(CreatureTimedEffect::MULTISHADOW) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::DUSTROBE)) {
        (void)set_dustrobe(creature, creature.get_timed_effect(CreatureTimedEffect::DUSTROBE) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::TIM_PASS_WALL)) {
        (void)set_pass_wall(creature, creature.get_timed_effect(CreatureTimedEffect::TIM_PASS_WALL) - 1, true);
    }

    if (creature.is_paralyzed()) {
        (void)bss.mod_paralysis(-1);
    }

    if (creature.is_confused()) {
        (void)bss.mod_confusion(-1);
    }

    if (creature.is_fearful()) {
        (void)bss.mod_fear(-1);
    }

    if (creature.is_fast()) {
        (void)mod_acceleration(creature, -1, true);
    }

    if (creature.is_decelerated()) {
        (void)bss.mod_deceleration(-1, true);
    }

    if (creature.is_protected_from_evil()) {
        BodyImprovement(creature).mod_protection(-1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::INVULNERABILITY)) {
        (void)set_invuln(creature, creature.get_timed_effect(CreatureTimedEffect::INVULNERABILITY) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::WRAITH_FORM)) {
        (void)set_wraith_form(creature, creature.get_timed_effect(CreatureTimedEffect::WRAITH_FORM) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::HERO)) {
        (void)set_hero(creature, creature.get_timed_effect(CreatureTimedEffect::HERO) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::BERSERK)) {
        (void)set_berserk(creature, creature.get_timed_effect(CreatureTimedEffect::BERSERK) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::BLESSED)) {
        (void)set_blessed(creature, creature.get_timed_effect(CreatureTimedEffect::BLESSED) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::SHIELD)) {
        (void)set_shield(creature, creature.get_timed_effect(CreatureTimedEffect::SHIELD) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::TSUBURERU)) {
        (void)set_leveling(creature, creature.get_timed_effect(CreatureTimedEffect::TSUBURERU) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::MAGICDEF)) {
        (void)set_magicdef(creature, creature.get_timed_effect(CreatureTimedEffect::MAGICDEF) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::TSUYOSHI)) {
        (void)set_tsuyoshi(creature, creature.get_timed_effect(CreatureTimedEffect::TSUYOSHI) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::OPPOSE_ACID)) {
        (void)set_oppose_acid(creature, creature.get_timed_effect(CreatureTimedEffect::OPPOSE_ACID) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::OPPOSE_ELEC)) {
        (void)set_oppose_elec(creature, creature.get_timed_effect(CreatureTimedEffect::OPPOSE_ELEC) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::OPPOSE_FIRE)) {
        (void)set_oppose_fire(creature, creature.get_timed_effect(CreatureTimedEffect::OPPOSE_FIRE) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::OPPOSE_COLD)) {
        (void)set_oppose_cold(creature, creature.get_timed_effect(CreatureTimedEffect::OPPOSE_COLD) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::OPPOSE_POIS)) {
        (void)set_oppose_pois(creature, creature.get_timed_effect(CreatureTimedEffect::OPPOSE_POIS) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::TIM_EMISSION)) {
        (void)set_tim_emission(creature, creature.get_timed_effect(CreatureTimedEffect::TIM_EMISSION) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::TIM_EXORCISM)) {
        (void)set_tim_exorcism(creature, creature.get_timed_effect(CreatureTimedEffect::TIM_EXORCISM) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::TIM_IMM_DARK)) {
        (void)set_tim_imm_dark(creature, creature.get_timed_effect(CreatureTimedEffect::TIM_IMM_DARK) - 1, true);
    }

    if (creature.get_timed_effect(CreatureTimedEffect::ULTIMATE_RESISTANCE)) {
        (void)set_ultimate_res(creature, creature.get_timed_effect(CreatureTimedEffect::ULTIMATE_RESISTANCE) - 1, true);
    }

    if (creature.is_poisoned()) {
        int adjust = adj_con_fix[creature.get_stat_index(A_CON)] + 1;
        (void)bss.mod_poison(-adjust);
    }

    if (creature.is_stunned()) {
        int adjust = adj_con_fix[creature.get_stat_index(A_CON)] + 1;
        (void)bss.mod_stun(-adjust);
    }

    if (creature.is_cut()) {
        short adjust = adj_con_fix[creature.get_stat_index(A_CON)] + 1;
        if (PlayerCut::get_rank(creature.get_timed_effect(CreatureTimedEffect::CUT)) == PlayerCutRank::MORTAL) {
            adjust = 0;
        }

        (void)bss.mod_cut(-adjust);
    }
}
