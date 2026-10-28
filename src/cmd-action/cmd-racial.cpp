/*
 * @brief クラス、種族、突然変異に関するコマンド処理
 * @author Hourier
 * @date 2022/02/24
 */

#include "cmd-action/cmd-racial.h"
#include "action/action-limited.h"
#include "action/mutation-execution.h"
#include "action/racial-execution.h"
#include "bot/bot-json-output.h"
#include "core/asking-player.h"
#include "core/window-redrawer.h"
#include "game-option/text-display-options.h"
#include "io/command-repeater.h"
#include "io/input-context.h"
#include "io/input-key-requester.h"
#include "main/sound-of-music.h"
#include "mutation/mutation-flag-types.h"
#include "player-base/player-class.h"
#include "player-info/race-info.h"
#include "player-info/samurai-data-type.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/player-damage.h"
#include "player/special-defense-types.h"
#include "racial/class-racial-switcher.h"
#include "racial/mutation-racial-selector.h"
#include "racial/race-racial-command-setter.h"
#include "racial/racial-util.h"
#include "status/action-setter.h"
#include "system/creature-entity.h"
#include "system/deferred-update-requests.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "view/display-util.h"
#include "world/world.h"
#include <fmt/format.h>
#include <string>

#define RC_PAGE_SIZE 18

#define RC_CANCEL true
#define RC_CONTINUE false

static void racial_power_display_cursor(rc_type *rc_ptr)
{
    auto y = rc_ptr->menu_line % RC_PAGE_SIZE;
    put_str(_(" 》 ", " >  "), 2 + y, 11);
}

static void racial_power_erase_cursor(rc_type *rc_ptr)
{
    auto y = rc_ptr->menu_line % RC_PAGE_SIZE;
    put_str(_("    ", "    "), 2 + y, 11);
}

/*!
 * @brief レイシャルパワー一覧を表示
 * @param creature クリーチャーへの参照
 * @param rc_ptr レイシャルパワー情報への参照ポインタ
 * @return キャンセルしたらRC_CANCEL、それ以外ならRC_CONTINUE
 * @details 一覧を表示していない (ページが -1) ときは何も描かない。
 */
static void racial_power_display_list(CreatureEntity &creature, rc_type *rc_ptr)
{
    if (rc_ptr->page < 0) {
        return;
    }

    TERM_LEN x = 11;
    std::string dummy;

    prt(_("                                   Lv   MP 失率 効果", "                                   Lv   MP Fail Effect"), 1, x);

    auto y = 0;
    for (; y < RC_PAGE_SIZE; y++) {
        auto ctr = RC_PAGE_SIZE * rc_ptr->page + y;
        if (ctr >= rc_ptr->power_count()) {
            break;
        }

        if (use_menu) {
            dummy = "    ";
        } else {
            dummy = fmt::format(" {:c}) ", I2AD(ctr));
        }

        auto &rpi = rc_ptr->power_desc[ctr];
        dummy.append(fmt::format("{:<30} {:2} {:4} {:3}% {}", fmt::bytes(str_substr(rpi.racial_name, 0, 30)), rpi.min_level, rpi.cost, 100 - racial_chance(creature, &rc_ptr->power_desc[ctr]), rpi.info));

        prt(dummy, 2 + y, x);
    }

    prt("", 2 + y, x);

    if (use_menu) {
        racial_power_display_cursor(rc_ptr);
    }

    // ページ送りで見られる全行を、表示と同じ値で出力する
    std::vector<BotPowerListRow> bot_rows;
    for (auto ctr = 0; ctr < rc_ptr->power_count(); ctr++) {
        const auto &rpi = rc_ptr->power_desc[ctr];
        bot_rows.push_back({ I2AD(ctr), ctr / RC_PAGE_SIZE, rpi.racial_name, rpi.min_level, rpi.cost, 100 - racial_chance(creature, &rc_ptr->power_desc[ctr]), rpi.info });
    }
    output_bot_json_power_list_snapshot(&static_cast<PlayerType &>(creature), "racial", bot_rows, rc_ptr->page, rc_ptr->browse_mode);
}

/*!
 * @brief レイシャルパワー選択用のプロンプトを作成する
 * @param rc_ptr レイシャルパワー情報への参照ポインタ
 */
static void racial_power_make_prompt(rc_type *rc_ptr)
{
    constexpr auto fmt_browse = _("(特殊能力 {:c}-{:c}, '*':一覧, '/'で使用, ESCで中断) どの能力について知りますか？", "(Powers {:c}-{:c}, *=List. /=Use, ESC=exit) Browse which power? ");
    constexpr auto fmt_use = _("(特殊能力 {:c}-{:c}, '*'で一覧, '/'で閲覧, ESCで中断) どの能力を使いますか？", "(Powers {:c}-{:c}, *=List, /=Browse, ESC=exit) Use which power? ");
    const auto last = I2AD(rc_ptr->power_count() - 1);
    rc_ptr->out_val = rc_ptr->browse_mode ? fmt::format(fmt_browse, I2A(0), last) : fmt::format(fmt_use, I2A(0), last);
}

/*!
 * @brief レイシャルパワー選択用のカーソル位置を進める
 * @param creature クリーチャーへの参照
 * @param rc_ptr レイシャルパワー情報への参照ポインタ
 * @param i カーソル増分
 */
static void racial_power_add_index(CreatureEntity &creature, rc_type *rc_ptr, int i)
{
    auto n = rc_ptr->menu_line + i;
    if (i < -1 || i > 1) {
        if (n < 0 || n >= rc_ptr->power_count()) {
            return;
        }
    }
    if (n < 0) {
        n = rc_ptr->power_count() - 1;
    }
    if (n >= rc_ptr->power_count()) {
        n = 0;
    }

    auto p = n / RC_PAGE_SIZE;
    racial_power_erase_cursor(rc_ptr);
    rc_ptr->menu_line = n;
    if (rc_ptr->page != p) {
        rc_ptr->page = p;
        screen_load();
        screen_save();
        racial_power_display_list(creature, rc_ptr);
    } else {
        racial_power_display_cursor(rc_ptr);
    }
}

/*!
 * @brief メニューによる選択のキーを処理する
 * @param rc_ptr レイシャルパワー情報への参照ポインタ
 * @return キャンセルならRC_CANCEL、そうでないならRC_CONTINUE
 */
static bool racial_power_interpret_menu_keys(CreatureEntity &creature, rc_type *rc_ptr)
{
    switch (rc_ptr->choice) {
    case '0':
        return RC_CANCEL;
    case '8':
    case 'k':
    case 'K':
        racial_power_add_index(creature, rc_ptr, -1);
        return RC_CONTINUE;
    case '2':
    case 'j':
    case 'J':
        racial_power_add_index(creature, rc_ptr, 1);
        return RC_CONTINUE;
    case '6':
    case 'l':
    case 'L':
        racial_power_add_index(creature, rc_ptr, RC_PAGE_SIZE);
        return RC_CONTINUE;
    case '4':
    case 'h':
    case 'H':
        racial_power_add_index(creature, rc_ptr, 0 - RC_PAGE_SIZE);
        return RC_CONTINUE;
    case 'x':
    case 'X':
    case '\r':
        rc_ptr->command_code = (COMMAND_CODE)rc_ptr->menu_line;
        rc_ptr->is_chosen = true;
        rc_ptr->ask = false;
        return RC_CONTINUE;
    case '/':
        rc_ptr->browse_mode = !rc_ptr->browse_mode;
        racial_power_make_prompt(rc_ptr);
        return RC_CONTINUE;
    default:
        return RC_CONTINUE;
    }
}

/*!
 * @brief メニューからの選択決定を処理
 * @param creature クリーチャーへの参照
 * @param rc_ptr レイシャルパワー情報への参照ポインタ
 * @return キャンセルしたらRC_CANCEL、それ以外ならRC_CONTINUE
 */
static bool racial_power_select_by_menu(CreatureEntity &creature, rc_type *rc_ptr)
{
    if (!use_menu || rc_ptr->choice == ' ') {
        return RC_CONTINUE;
    }

    if (racial_power_interpret_menu_keys(creature, rc_ptr)) {
        return RC_CANCEL;
    }

    if (rc_ptr->menu_line > rc_ptr->power_count()) {
        rc_ptr->menu_line -= rc_ptr->power_count();
    }

    return RC_CONTINUE;
}

/*!
 * @brief レイシャルパワーの選択を解釈
 * @param creature クリーチャーへの参照
 * @param rc_ptr レイシャルパワー情報への参照ポインタ
 * @return コマンド選択していたらtrue、していなかったらfalse
 */
static bool racial_power_interpret_choise(CreatureEntity &creature, rc_type *rc_ptr)
{
    if (use_menu) {
        return false;
    }

    if (rc_ptr->choice == ' ' || rc_ptr->choice == '*') {
        rc_ptr->page++;
        if (rc_ptr->page > rc_ptr->max_page) {
            rc_ptr->page = 0;
        }
        screen_load();
        screen_save();
        racial_power_display_list(creature, rc_ptr);
        return false;
    }

    if (rc_ptr->choice == '/') {
        rc_ptr->browse_mode = !rc_ptr->browse_mode;
        racial_power_make_prompt(rc_ptr);
        return false;
    }

    if (rc_ptr->choice == '?') {
        return true;
    }

    return true;
}

static void decide_racial_command(rc_type *rc_ptr)
{
    if (use_menu) {
        return;
    }

    if (rc_ptr->choice == '\r' && rc_ptr->power_count() == 1) {
        rc_ptr->choice = 'a';
    }

    if (!isalpha(rc_ptr->choice)) {
        rc_ptr->ask = false;
        rc_ptr->command_code = rc_ptr->choice - '0' + 26;
        return;
    }

    rc_ptr->ask = (isupper(rc_ptr->choice));
    if (rc_ptr->ask) {
        rc_ptr->choice = (char)tolower(rc_ptr->choice);
    }

    rc_ptr->command_code = (islower(rc_ptr->choice) ? A2I(rc_ptr->choice) : -1);
}

static bool ask_invoke_racial_power(rc_type *rc_ptr)
{
    if ((rc_ptr->command_code < 0) || (rc_ptr->command_code >= rc_ptr->power_count())) {
        bell();
        return false;
    }

    if (!rc_ptr->ask) {
        return true;
    }

    const auto prompt = fmt::format(_("{}を使いますか？ ", "Use {}? "), rc_ptr->power_desc[rc_ptr->command_code].racial_name);
    return input_check(prompt);
}

static void racial_power_display_explanation(CreatureEntity &creature, rc_type *rc_ptr)
{
    auto &rpi = rc_ptr->power_desc[rc_ptr->command_code];

    term_erase(12, 21);
    term_erase(12, 20);
    term_erase(12, 19);
    term_erase(12, 18);
    term_erase(12, 17);
    term_erase(12, 16);
    display_wrap_around(rpi.text, 62, 17, 15);

    constexpr auto prompt = _("何かキーを押して下さい。", "Hit any key.");
    prt(prompt, 0, 0);
    (void)input_key_with_context(InputKind::ANY_KEY, prompt);

    screen_load();
    screen_save();
    racial_power_display_list(creature, rc_ptr);
    rc_ptr->is_chosen = false;
}

/*!
 * @brief レイシャルパワー選択処理のメインループ
 * @param creature クリーチャーへの参照
 * @param rc_ptr レイシャルパワー情報への参照ポインタ
 * @return コマンド選択したらRC_CONTINUE、キャンセルしたらRC_CANCEL
 */
static bool racial_power_process_input(CreatureEntity &creature, rc_type *rc_ptr)
{
    rc_ptr->choice = (always_show_list || use_menu) ? ESCAPE : 1;
    const auto first_label = I2A(0);
    const auto last_label = I2AD(rc_ptr->power_count() - 1);

    while (true) {
        if (rc_ptr->choice == ESCAPE) {
            rc_ptr->choice = ' ';
        } else {
            // ページが -1 なら一覧を表示していない。max_page のページは行が無く見出しだけを描く (一覧を隠した状態)
            const auto is_list_shown = rc_ptr->page >= 0 && rc_ptr->page < rc_ptr->max_page;
            const auto choice = input_spell_choice(rc_ptr->out_val, { .first_label = first_label, .last_label = last_label, .list = is_list_shown, .browse = rc_ptr->browse_mode, .menu = use_menu });
            if (!choice.has_value()) {
                return true;
            }

            rc_ptr->choice = choice.value();
        }

        if (racial_power_select_by_menu(creature, rc_ptr) == RC_CANCEL) {
            return RC_CANCEL;
        }

        if (!rc_ptr->is_chosen && racial_power_interpret_choise(creature, rc_ptr)) {
            decide_racial_command(rc_ptr);
            if (ask_invoke_racial_power(rc_ptr)) {
                rc_ptr->is_chosen = true;
            }
        }

        if (rc_ptr->is_chosen) {
            if (rc_ptr->browse_mode) {
                racial_power_display_explanation(creature, rc_ptr);
            } else {
                break;
            }
        }
    }

    return RC_CONTINUE;
}

/*!
 * @brief レイシャル/クラスパワー選択を処理
 * @param creature クリーチャーへの参照
 * @param rc_ptr レイシャルパワー情報への参照ポインタ
 * @return コマンド選択したらRC_CONTINUE、キャンセルしたらRC_CANCEL
 */
static bool racial_power_select_power(CreatureEntity &creature, rc_type *rc_ptr)
{
    const auto code = repeat_pull();
    if (code) {
        rc_ptr->command_code = *code;
        if ((rc_ptr->command_code >= 0) && (rc_ptr->command_code < rc_ptr->power_count())) {
            return true;
        }
    }

    // 一覧は選び終えるまで消さないので、この関数を抜けるまで登録する
    BotDrawnScreenScope bot_screen;
    screen_save();

    if (use_menu) {
        racial_power_display_list(creature, rc_ptr);
    }

    auto canceled = racial_power_process_input(creature, rc_ptr) == RC_CANCEL;

    screen_load();

    if (canceled) {
        return RC_CANCEL;
    }

    repeat_push(rc_ptr->command_code);
    return RC_CONTINUE;
}

/*!
 * @brief レイシャルパワーの使用を試みる
 * @param creature クリーチャーへの参照
 * @param rc_ptr レイシャルパワー情報への参照ポインタ
 * @details
 * 戻り値の代わりにrc_ptr->castに使用の有無を入れる。
 */
static void racial_power_cast_power(CreatureEntity &creature, rc_type *rc_ptr)
{
    auto *rpi_ptr = &rc_ptr->power_desc[rc_ptr->command_code];

    switch (check_racial_level(creature, rpi_ptr)) {
    case RACIAL_SUCCESS:
        if (rpi_ptr->number < 0) {
            rc_ptr->cast = exe_racial_power(creature, rpi_ptr->number);
        } else {
            rc_ptr->cast = exe_mutation_power(creature, i2enum<PlayerMutationType>(rpi_ptr->number));
        }
        break;
    case RACIAL_FAILURE:
        rc_ptr->cast = true;
        break;
    case RACIAL_CANCEL:
        rc_ptr->cast = false;
        break;
    }
}

/*!
 * @brief レイシャルパワーのコストを減らす
 * @param creature クリーチャーへの参照
 * @param rc_ptr レイシャルパワー情報への参照ポインタ
 * @return コストを減らしたらtrue、減らさなかったらfalse
 * @details
 * MPが足りない場合はHPを減らす。
 * 戻り値はHP/MPの再描画が必要か判定するのに使用。
 */
static bool racial_power_reduce_mana(CreatureEntity &creature, rc_type *rc_ptr)
{
    int racial_cost = rc_ptr->power_desc[rc_ptr->command_code].racial_cost;
    if (racial_cost == 0) {
        return false;
    }

    int actual_racial_cost = racial_cost / 2 + randint1(racial_cost / 2);

    if (creature.get_current_mp() >= actual_racial_cost) {
        creature.sub_current_mp(actual_racial_cost);
    } else {
        actual_racial_cost -= creature.get_current_mp();
        creature.set_current_mp(0);
        take_hit(creature, DAMAGE_USELIFE, actual_racial_cost, _("過度の集中", "concentrating too hard"));
    }

    return true;
}

/*!
 * @brief レイシャル・パワーコマンドのメインルーチン / Allow user to choose a power (racial / mutation) to activate
 * @param creature クリーチャーへの参照
 */
void do_cmd_racial_power(CreatureEntity &creature)
{
    if (AngbandWorld::get_instance().is_wild_mode()) {
        return;
    }

    PlayerEnergy energy(creature);
    if (cmd_limit_confused(creature)) {
        energy.reset_player_turn();
        return;
    }

    CreatureClass(creature).break_samurai_stance({ SamuraiStanceType::MUSOU, SamuraiStanceType::KOUKIJIN });

    auto tmp_r = rc_type(creature);
    auto *rc_ptr = &tmp_r;

    switch_class_racial(creature, rc_ptr);

    if (creature.get_mimic_form() != MimicKindType::NONE) {
        set_mimic_racial_command(creature, rc_ptr);
    } else {
        set_race_racial_command(creature, rc_ptr);
    }

    select_mutation_racial(creature, rc_ptr);

    if (rc_ptr->power_count() == 0) {
        msg_print(_("特殊能力はありません。", "You have no special powers."));
        return;
    }

    rc_ptr->max_page = 1 + (rc_ptr->power_count() - 1) / RC_PAGE_SIZE;
    rc_ptr->page = use_menu ? 0 : -1;
    racial_power_make_prompt(rc_ptr);

    if (racial_power_select_power(creature, rc_ptr) == RC_CONTINUE) {
        racial_power_cast_power(creature, rc_ptr);
    }

    if (!rc_ptr->cast) {
        energy.reset_player_turn();
        return;
    }

    if (!racial_power_reduce_mana(creature, rc_ptr)) {
        return;
    }

    creature.plus_incident_tree("USE_RACIAL", 1);
    DeferredUpdateRequests::get_instance().request_hp_mp_redrawing();
}
