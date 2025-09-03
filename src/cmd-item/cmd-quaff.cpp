/*!
 * @brief プレイヤーの飲むコマンド実装
 * @date 2018/09/07
 * @author deskull
 */

#include "cmd-item/cmd-quaff.h"
#include "action/action-limited.h"
#include "floor/floor-object.h"
#include "object-hook/hook-expendable.h"
#include "object-use/quaff/quaff-execution.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "player-base/player-class.h"
#include "player-info/samurai-data-type.h"
#include "player/attack-defense-types.h"
#include "player/special-defense-types.h"
#include "realm/realm-hex-numbers.h"
#include "spell-realm/spells-hex.h"
#include "status/action-setter.h"
#include "system/player-type-definition.h"
#include "timed-effect/player-confusion.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "status/bad-status-setter.h"
#include "world/world.h"

/*!
 * @brief 薬を飲むコマンドのメインルーチン /
 * Quaff some potion (from the pack or floor)
 */
void do_cmd_quaff_potion(PlayerType *player_ptr)
{
    if (AngbandWorld::get_instance().is_wild_mode()) {
        return;
    }

    if (!SpellHex(player_ptr).is_spelling_specific(HEX_INHALE) && cmd_limit_arena(player_ptr)) {
        return;
    }

    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU, SamuraiStanceType::KOUKIJIN });

    constexpr auto q = _("どの薬を飲みますか? ", "Quaff which potion? ");
    constexpr auto s = _("飲める薬がない。", "You have no potions to quaff.");

    short i_idx;
    if (!choose_object(player_ptr, &i_idx, q, s, (USE_INVEN | USE_FLOOR), FuncItemTester(item_tester_hook_quaff, player_ptr))) {
        return;
    }

    ObjectQuaffEntity(player_ptr).execute(i_idx);
}

/*!
 * @brief 薬を直腸吸収するコマンドのメインルーチン /
 * Absorb some potion through rectal route (from the pack or floor)
 */
void do_cmd_rectal_absorption(PlayerType *player_ptr)
{
    if (AngbandWorld::get_instance().is_wild_mode()) {
        return;
    }

    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU, SamuraiStanceType::KOUKIJIN });

    constexpr auto q = _("どの薬を直腸吸収しますか? ", "Which potion do you want to absorb rectally? ");
    constexpr auto s = _("直腸吸収できる薬がない。", "You have no potions for rectal absorption.");

    short i_idx;
    if (!choose_object(player_ptr, &i_idx, q, s, (USE_INVEN | USE_FLOOR), FuncItemTester(item_tester_hook_quaff, player_ptr))) {
        return;
    }

    // 変態行為の実行メッセージ
    msg_print(_("あなたは薬を肛門に注入した...なんという変態行為！", "You insert the potion into your rectum... What a perverted act!"));

    // 確率で失敗する（恥ずかしさによる）
    if (one_in_(10)) {
        msg_print(_("恥ずかしさのあまり失敗してしまった。", "You failed due to embarrassment."));
        return;
    }

    // 通常の薬効果を発動（ただし効果は若干異なる可能性）
    ObjectQuaffEntity(player_ptr).execute(i_idx);

    // 変態行為による追加効果
    msg_print(_("あなたは異常な快感を感じている...", "You feel abnormal pleasure..."));

    // 混乱状態になる可能性
    if (one_in_(3)) {
        msg_print(_("変態行為により精神が混乱した！", "Your mind is confused by the perverted act!"));
        (void)BadStatusSetter(player_ptr).set_confusion(randint1(30) + 30);
    }
}
