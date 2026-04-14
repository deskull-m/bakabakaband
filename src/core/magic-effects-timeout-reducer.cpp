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
#include "timed-effect/timed-effects.h"

/*!
 * @brief 10ゲームターンが進行するごとに魔法効果の残りターンを減らしていく処理
 * / Handle timeout every 10 game turns
 */
void reduce_magic_effects_timeout(CreatureEntity &creature)
{
    if (creature.tim_mimic) {
        (void)set_mimic(creature, creature.tim_mimic - 1, creature.mimic_form, true);
    }

    BadStatusSetter bss(creature);
    const auto effects = creature.effects();
    if (effects->hallucination().is_hallucinated()) {
        (void)bss.mod_hallucination(-1);
    }

    if (effects->blindness().is_blind()) {
        (void)bss.mod_blindness(-1);
    }

    if (const auto remaining = creature.get_remaining_tim_invis(); remaining > 0) {
        (void)set_tim_invis(creature, remaining - 1, true);
    }

    if (creature.suppress_multi_reward) {
        creature.suppress_multi_reward = false;
    }

    if (const auto remaining = creature.get_remaining_tim_esp(); remaining > 0) {
        (void)set_tim_esp(creature, remaining - 1, true);
    }

    if (creature.ele_attack) {
        creature.ele_attack--;
        if (!creature.ele_attack) {
            set_ele_attack(creature, 0, 0);
        }
    }

    if (creature.ele_immune) {
        creature.ele_immune--;
        if (!creature.ele_immune) {
            set_ele_immune(creature, 0, 0);
        }
    }

    if (const auto remaining = creature.get_remaining_tim_infra(); remaining > 0) {
        (void)set_tim_infra(creature, remaining - 1, true);
    }

    if (const auto remaining = creature.get_remaining_tim_stealth(); remaining > 0) {
        (void)set_tim_stealth(creature, remaining - 1, true);
    }

    if (creature.tim_levitation) {
        (void)set_tim_levitation(creature, creature.tim_levitation - 1, true);
    }

    if (creature.tim_sh_touki) {
        (void)set_tim_sh_force(creature, creature.tim_sh_touki - 1, true);
    }

    if (creature.tim_sh_fire) {
        (void)set_tim_sh_fire(creature, creature.tim_sh_fire - 1, true);
    }

    if (creature.tim_sh_holy) {
        (void)set_tim_sh_holy(creature, creature.tim_sh_holy - 1, true);
    }

    if (creature.tim_eyeeye) {
        (void)set_tim_eyeeye(creature, creature.tim_eyeeye - 1, true);
    }

    if (creature.resist_magic) {
        (void)set_resist_magic(creature, creature.resist_magic - 1, true);
    }

    if (const auto remaining = creature.get_remaining_tim_regen(); remaining > 0) {
        (void)set_tim_regen(creature, remaining - 1, true);
    }

    if (creature.tim_res_nether) {
        (void)set_tim_res_nether(creature, creature.tim_res_nether - 1, true);
    }

    if (creature.tim_res_lite) {
        (void)set_tim_res_lite(creature, creature.tim_res_lite - 1, true);
    }

    if (creature.tim_res_dark) {
        (void)set_tim_res_dark(creature, creature.tim_res_dark - 1, true);
    }

    if (creature.tim_res_fear) {
        (void)set_tim_res_fear(creature, creature.tim_res_fear - 1, true);
    }

    if (creature.tim_res_time) {
        (void)set_tim_res_time(creature, creature.tim_res_time - 1, true);
    }

    if (creature.tim_reflect) {
        (void)set_tim_reflect(creature, creature.tim_reflect - 1, true);
    }

    if (creature.multishadow) {
        (void)set_multishadow(creature, creature.multishadow - 1, true);
    }

    if (creature.dustrobe) {
        (void)set_dustrobe(creature, creature.dustrobe - 1, true);
    }

    if (creature.tim_pass_wall) {
        (void)set_pass_wall(creature, creature.tim_pass_wall - 1, true);
    }

    if (effects->paralysis().is_paralyzed()) {
        (void)bss.mod_paralysis(-1);
    }

    if (creature.is_confused()) {
        (void)bss.mod_confusion(-1);
    }

    if (creature.is_fearful()) {
        (void)bss.mod_fear(-1);
    }

    if (effects->acceleration().is_fast()) {
        (void)mod_acceleration(creature, -1, true);
    }

    if (effects->deceleration().is_slow()) {
        (void)bss.mod_deceleration(-1, true);
    }

    if (effects->protection().is_protected()) {
        BodyImprovement(creature).mod_protection(-1, true);
    }

    if (creature.invuln) {
        (void)set_invuln(creature, creature.invuln - 1, true);
    }

    if (const auto remaining = creature.get_remaining_wraith_form(); remaining > 0) {
        (void)set_wraith_form(creature, remaining - 1, true);
    }

    if (const auto remaining = creature.get_remaining_hero(); remaining > 0) {
        (void)set_hero(creature, remaining - 1, true);
    }

    if (const auto remaining = creature.get_remaining_berserk(); remaining > 0) {
        (void)set_berserk(creature, remaining - 1, true);
    }

    if (const auto remaining = creature.get_remaining_blessed(); remaining > 0) {
        (void)set_blessed(creature, remaining - 1, true);
    }

    if (const auto remaining = creature.get_remaining_shield(); remaining > 0) {
        (void)set_shield(creature, remaining - 1, true);
    }

    if (creature.tsubureru) {
        (void)set_leveling(creature, creature.tsubureru - 1, true);
    }

    if (creature.magicdef) {
        (void)set_magicdef(creature, creature.magicdef - 1, true);
    }

    if (const auto remaining = creature.get_remaining_tsuyoshi(); remaining > 0) {
        (void)set_tsuyoshi(creature, remaining - 1, true);
    }

    if (const auto remaining = creature.get_remaining_oppose_acid(); remaining > 0) {
        (void)set_oppose_acid(creature, remaining - 1, true);
    }

    if (const auto remaining = creature.get_remaining_oppose_elec(); remaining > 0) {
        (void)set_oppose_elec(creature, remaining - 1, true);
    }

    if (const auto remaining = creature.get_remaining_oppose_fire(); remaining > 0) {
        (void)set_oppose_fire(creature, remaining - 1, true);
    }

    if (const auto remaining = creature.get_remaining_oppose_cold(); remaining > 0) {
        (void)set_oppose_cold(creature, remaining - 1, true);
    }

    if (const auto remaining = creature.get_remaining_oppose_pois(); remaining > 0) {
        (void)set_oppose_pois(creature, remaining - 1, true);
    }

    if (creature.tim_emission) {
        (void)set_tim_emission(creature, creature.tim_emission - 1, true);
    }

    if (creature.tim_exorcism) {
        (void)set_tim_exorcism(creature, creature.tim_exorcism - 1, true);
    }

    if (creature.tim_imm_dark) {
        (void)set_tim_imm_dark(creature, creature.tim_imm_dark - 1, true);
    }

    if (const auto remaining = creature.get_remaining_ultimate_resistance(); remaining > 0) {
        (void)set_ultimate_res(creature, remaining - 1, true);
    }

    if (effects->poison().is_poisoned()) {
        int adjust = adj_con_fix[creature.stat_index[A_CON]] + 1;
        (void)bss.mod_poison(-adjust);
    }

    if (effects->stun().is_stunned()) {
        int adjust = adj_con_fix[creature.stat_index[A_CON]] + 1;
        (void)bss.mod_stun(-adjust);
    }

    auto &player_cut = effects->cut();
    if (player_cut.is_cut()) {
        short adjust = adj_con_fix[creature.stat_index[A_CON]] + 1;
        if (player_cut.get_rank() == PlayerCutRank::MORTAL) {
            adjust = 0;
        }

        (void)bss.mod_cut(-adjust);
    }
}
