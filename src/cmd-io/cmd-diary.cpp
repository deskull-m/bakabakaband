#include "cmd-io/cmd-diary.h"
#include "core/asking-player.h"
#include "core/show-file.h"
#include "game-option/play-record-options.h"
#include "io/files-util.h"
#include "io/input-key-acceptor.h"
#include "io/record-play-movie.h"
#include "io/write-diary.h"
#include "main/sound-of-music.h"
#include "player-base/player-class.h"
#include "player/player-personality.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "util/angband-files.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <fmt/format.h>
#include <sstream>
#include <string>

/*!
 * @brief 日記のタイトル表記と内容出力
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void display_diary(PlayerType *player_ptr)
{
    const auto subtitle_candidates = PlayerClass(player_ptr).get_subtitle_candidates();
    const auto choice = Rand_external(subtitle_candidates.size());
    const auto &subtitle = subtitle_candidates[choice];
#ifdef JP
    const auto diary_title = format("「%s%s%sの伝説 -%s-」", ap_ptr->title.data(), ap_ptr->no ? "の" : "", player_ptr->name, subtitle.data());
#else
    const auto diary_title = format("Legend of %s %s '%s'", ap_ptr->title.data(), player_ptr->name, subtitle.data());
#endif

    std::stringstream ss;
    ss << _("playrecord-", "playrec-") << savefile_base.string() << ".txt";
    const auto path = path_build(ANGBAND_DIR_USER, ss.str());
    FileDisplayer(player_ptr->name).display(false, path.string(), -1, 0, diary_title);
}

/*!
 * @brief 日記に任意の内容を表記するコマンドのメインルーチン /
 */
static void add_diary_note(const FloorType &floor)
{
    const auto input_str = input_string(_("内容: ", "diary note: "), 1000);
    if (input_str) {
        exe_write_diary(floor, DiaryKind::DESCRIPTION, 0, *input_str);
    }
}

/*!
 * @brief 最後に取得したアイテムの情報を日記に追加するメインルーチン /
 */
static void do_cmd_last_get(const FloorType &floor)
{
    if (record_item_name.empty()) {
        return;
    }

    const auto record = fmt::format(_("{}の入手を記録します。", "Do you really want to record getting {}? "), record_item_name);
    if (!input_check(record)) {
        return;
    }

    auto &world = AngbandWorld::get_instance();
    const auto turn_tmp = world.game_turn;
    world.game_turn = record_turn;
    const auto mes = fmt::format(_("{}を手に入れた。", "discover {}."), record_item_name);
    exe_write_diary(floor, DiaryKind::DESCRIPTION, 0, mes);
    world.game_turn = turn_tmp;
}

/*!
 * @brief ファイル中の全日記記録を消去する /
 */
static void do_cmd_erase_diary()
{
    if (!input_check(_("本当に記録を消去しますか？", "Do you really want to delete all your records? "))) {
        return;
    }

    std::stringstream ss;
    ss << _("playrecord-", "playrec-") << savefile_base.string() << ".txt";
    const auto path = path_build(ANGBAND_DIR_USER, ss.str());
    fd_kill(path);

    auto *fff = angband_fopen(path, FileOpenMode::WRITE);
    if (fff) {
        angband_fclose(fff);
        msg_format(_("記録を消去しました。", "deleted record."));
    } else {
        const auto &filename = path.string();
        msg_format(_("%s の消去に失敗しました。", "failed to delete %s."), filename.data());
    }

    msg_print(nullptr);
}

/*!
 * @brief 日記コマンド
 * @param crerature_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_diary(PlayerType *player_ptr)
{
    screen_save();
    TermCenteredOffsetSetter tcos(MAIN_TERM_MIN_COLS, MAIN_TERM_MIN_ROWS);
    const auto &floor = *player_ptr->current_floor_ptr;
    while (true) {
        term_clear();
        prt(_("[ 記録の設定 ]", "[ Play Record ]"), 2, 0);
        prt(_("(1) 記録を見る", "(1) Display your record"), 4, 5);
        prt(_("(2) 文章を記録する", "(2) Add record"), 5, 5);
        prt(_("(3) 直前に入手又は鑑定したものを記録する", "(3) Record the last item you got or identified"), 6, 5);
        prt(_("(4) 記録を消去する", "(4) Delete your record"), 7, 5);
        prt(_("(R) プレイ動画を記録する/中止する", "(R) Record playing movie / or stop it"), 9, 5);
        prt(_("コマンド:", "Command: "), 18, 0);
        int i = inkey();
        if (i == ESCAPE) {
            break;
        }

        switch (i) {
        case '1':
            display_diary(player_ptr);
            break;
        case '2':
            add_diary_note(floor);
            break;
        case '3':
            do_cmd_last_get(floor);
            break;
        case '4':
            do_cmd_erase_diary();
            break;
        case 'r':
        case 'R':
            screen_load();
            prepare_movie_hooks(player_ptr);
            return;
        default:
            bell();
        }

        msg_erase();
    }

    screen_load();
}
