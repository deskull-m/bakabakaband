#include "mind/mind-blue-mage.h"
#include "action/action-limited.h"
#include "avatar/avatar.h"
#include "blue-magic/blue-magic-caster.h"
#include "blue-magic/learnt-power-getter.h"
#include "core/asking-player.h"
#include "core/window-redrawer.h"
#include "game-option/disturbance-options.h"
#include "game-option/input-options.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-race/race-ability-mask.h"
#include "mspell/monster-power-table.h"
#include "player-status/player-energy.h"
#include "player/player-status-table.h"
#include "realm/realm-types.h"
#include "spell/spell-info.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"

/*!
 * @brief 青魔法コマンドのメインルーチン /
 * do_cmd_cast calls this function if the player's class is 'Blue-Mage'.
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
bool do_cmd_cast_learned(PlayerType *player_ptr)
{
    if (cmd_limit_confused(player_ptr)) {
        return false;
    }

    auto selected_spell = get_learned_power(player_ptr);
    if (!selected_spell.has_value()) {
        return false;
    }

    const auto &spell = monster_powers.at(*selected_spell);
    const auto need_mana = mod_need_mana(player_ptr, spell.smana, 0, RealmType::NONE);
    if (need_mana > player_ptr->csp) {
        msg_print(_("ＭＰが足りません。", "You do not have enough mana to use this power."));
        if (!over_exert) {
            return false;
        }

        if (!input_check(_("それでも挑戦しますか? ", "Attempt it anyway? "))) {
            return false;
        }
    }

    const auto chance = calculate_blue_magic_failure_probability(player_ptr, spell, need_mana);

    if (evaluate_percent(chance)) {
        if (flush_failure) {
            flush();
        }

        msg_print(_("魔法をうまく唱えられなかった。", "You failed to concentrate hard enough!"));
        sound(SoundKind::FAIL);
        if (RF_ABILITY_SUMMON_MASK.has(*selected_spell)) {
            cast_learned_spell(player_ptr, *selected_spell, false);
        }
    } else {
        sound(SoundKind::ZAP);
        if (!cast_learned_spell(player_ptr, *selected_spell, true)) {
            return false;
        }
    }

    if (need_mana <= player_ptr->csp) {
        player_ptr->csp -= need_mana;
    } else {
        int oops = need_mana;
        player_ptr->csp = 0;
        player_ptr->csp_frac = 0;
        msg_print(_("精神を集中しすぎて気を失ってしまった！", "You faint from the effort!"));
        (void)BadStatusSetter(player_ptr).mod_paralysis(randnum1<short>(5 * oops + 1));
        chg_virtue(player_ptr, Virtue::KNOWLEDGE, -10);
        if (one_in_(2)) {
            const auto perm = one_in_(4);
            msg_print(_("体を悪くしてしまった！", "You have damaged your health!"));
            (void)dec_stat(player_ptr, A_CON, 15 + randint1(10), perm);
        }
    }

    PlayerEnergy(player_ptr).set_player_turn_energy(100);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::MP);
    static constexpr auto flags = {
        SubWindowRedrawingFlag::PLAYER,
        SubWindowRedrawingFlag::SPELL,
    };
    rfu.set_flags(flags);
    return true;
}
