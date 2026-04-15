#include "mspell/mspell-dispel.h"
#include "blue-magic/blue-magic-checker.h"
#include "core/speed-table.h"
#include "core/window-redrawer.h"
#include "mind/mind-force-trainer.h"
#include "mind/mind-magic-resistance.h"
#include "mind/mind-mirror-master.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "mspell/mspell-result.h"
#include "mspell/mspell-util.h"
#include "player-info/race-info.h"
#include "player/attack-defense-types.h"
#include "realm/realm-song-numbers.h"
#include "spell-realm/spells-craft.h"
#include "spell-realm/spells-crusade.h"
#include "spell-realm/spells-demon.h"
#include "spell-realm/spells-hex.h"
#include "spell-realm/spells-song.h"
#include "status/bad-status-setter.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "status/shape-changer.h"
#include "status/sight-setter.h"
#include "status/temporary-resistance.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/redrawing-flags-updater.h"
#include "view/display-messages.h"

/*!
 * @brief プレイヤーに魔力消去効果を与える。
 */
static void dispel_player(CreatureEntity &creature)
{
    (void)set_acceleration(creature, 0, true);
    set_lightspeed(creature, 0, true);
    (void)BadStatusSetter(creature).set_deceleration(0, true);
    (void)set_shield(creature, 0, true);
    (void)set_blessed(creature, 0, true);
    (void)set_tsuyoshi(creature, 0, true);
    (void)set_hero(creature, 0, true);
    (void)set_berserk(creature, 0, true);
    BodyImprovement(creature).set_protection(0, true);
    (void)set_invuln(creature, 0, true);
    (void)set_wraith_form(creature, 0, true);
    (void)set_pass_wall(creature, 0, true);
    (void)set_tim_res_nether(creature, 0, true);
    (void)set_tim_res_time(creature, 0, true);
    (void)set_tim_res_lite(creature, 0, true);
    (void)set_tim_res_dark(creature, 0, true);
    (void)set_tim_res_fear(creature, 0, true);
    (void)set_tim_reflect(creature, 0, true);
    (void)set_multishadow(creature, 0, true);
    (void)set_dustrobe(creature, 0, true);

    (void)set_tim_invis(creature, 0, true);
    (void)set_tim_infra(creature, 0, true);
    (void)set_tim_esp(creature, 0, true);
    (void)set_tim_regen(creature, 0, true);
    (void)set_tim_stealth(creature, 0, true);
    (void)set_tim_levitation(creature, 0, true);
    (void)set_tim_sh_force(creature, 0, true);
    (void)set_tim_sh_fire(creature, 0, true);
    (void)set_tim_sh_holy(creature, 0, true);
    (void)set_tim_eyeeye(creature, 0, true);
    (void)set_magicdef(creature, 0, true);
    (void)set_resist_magic(creature, 0, true);
    (void)set_oppose_acid(creature, 0, true);
    (void)set_oppose_elec(creature, 0, true);
    (void)set_oppose_fire(creature, 0, true);
    (void)set_oppose_cold(creature, 0, true);
    (void)set_oppose_pois(creature, 0, true);
    (void)set_ultimate_res(creature, 0, true);
    (void)set_mimic(creature, 0, MimicKindType::NONE, true);
    (void)set_ele_attack(creature, 0, 0);
    (void)set_ele_immune(creature, 0, 0);
    (void)set_tim_emission(creature, 0, true);
    (void)set_tim_exorcism(creature, 0, true);
    (void)set_tim_imm_dark(creature, 0, true);

    if (creature.special_attack & ATTACK_CONFUSE) {
        creature.special_attack &= ~(ATTACK_CONFUSE);
        msg_print(_("手の輝きがなくなった。", "Your hands stop glowing."));
    }

    auto song_interruption = music_singing_any(creature);
    auto spellhex_interruption = SpellHex(creature).is_spelling_any();

    if (song_interruption || spellhex_interruption) {
        if (song_interruption) {
            set_interrupting_song_effect(creature, get_singing_song_effect(creature));
            set_singing_song_effect(creature, MUSIC_NONE);
            msg_print(_("歌が途切れた。", "Your singing is interrupted."));
        }
        if (spellhex_interruption) {
            SpellHex(creature).interrupt_spelling();
            msg_print(_("呪文が途切れた。", "Your casting is interrupted."));
        }

        creature.action = ACTION_NONE;
        auto &rfu = RedrawingFlagsUpdater::get_instance();
        static constexpr auto flags_srf = {
            StatusRecalculatingFlag::BONUS,
            StatusRecalculatingFlag::HP,
            StatusRecalculatingFlag::MONSTER_STATUSES,
        };
        rfu.set_flags(flags_srf);
        static constexpr auto flags_mwrf = {
            MainWindowRedrawingFlag::MAP,
            MainWindowRedrawingFlag::TIMED_EFFECT,
            MainWindowRedrawingFlag::ACTION,
        };
        rfu.set_flags(flags_mwrf);
        static constexpr auto flags_swrf = {
            SubWindowRedrawingFlag::OVERHEAD,
            SubWindowRedrawingFlag::DUNGEON,
        };
        rfu.set_flags(flags_swrf);
        creature.energy_need += ENERGY_NEED();
    }
}

/*!
 * @brief RF4_DISPELの処理。魔力消去。 /
 * @param m_idx 呪文を唱えるモンスターID
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF4_DISPEL(MONSTER_IDX m_idx, CreatureEntity &creature, MONSTER_IDX t_idx, int target_type)
{
    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    mspell_cast_msg_blind msg(_("%s^が何かを力強くつぶやいた。", "%s^ mumbles powerfully."),
        _("%s^が魔力消去の呪文を念じた。", "%s^ invokes a dispel magic."), _("%s^が%sに対して魔力消去の呪文を念じた。", "%s^ invokes a dispel magic at %s."));

    monspell_message(creature, m_idx, t_idx, msg, target_type);

    if (target_type == MONSTER_TO_PLAYER) {
        dispel_player(creature);
        if (creature.riding) {
            dispel_monster_status(creature, creature.riding);
        }

        if (creature.is_echizen()) {
            msg_print(_("やりやがったな！", ""));
        } else if (creature.is_chargeman()) {
            if (randint0(2) == 0) {
                msg_print(_("ジュラル星人め！", ""));
            } else {
                msg_print(_("弱い者いじめは止めるんだ！", ""));
            }
        } else if (creature.is_tough()) {
            msg_print(_("う わ あ あ あ あ あ あ あ あ", ""));
        }

        return res;
    }
    const auto &floor = *creature.get_floor();
    const auto &target = floor.get_monster(t_idx);
    if (target_type == MONSTER_TO_MONSTER) {
        if (target.is_riding()) {
            dispel_player(creature);
        }

        dispel_monster_status(creature, t_idx);
    }

    return res;
}
