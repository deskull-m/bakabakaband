/*!
 * @file report.c
 * @brief スコアサーバ転送機能の実装
 * @date 2014/07/14
 * @author Bakabakaband Team
 */

#include "io/report.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "core/visuals-reseter.h"
#include "game-option/special-options.h"
#include "io-dump/character-dump.h"
#include "io/input-key-acceptor.h"
#include "mind/mind-elementalist.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "player/player-personality.h"
#include "player/player-realm.h"
#include "player/player-status.h"
#include "system/angband-system.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-record.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/floor/floor-info.h"
#include "system/inner-game-data.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/system-variables.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "util/angband-files.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <algorithm>
#include <fmt/format.h>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

std::string screen_dump;

#ifdef WORLD_SCORE

/*
 * internet resource value
 */
#define HTTP_TIMEOUT 30 /*!< デフォルトのタイムアウト時間(秒) / Timeout length (second) */

#ifdef JP
constexpr auto SCORE_SERVER_SCHEME_HOST = ""; /*!< スコアサーバホスト */
constexpr auto SCORE_SERVER_PATH = ""; /*< スコアサーバパス */
#endif

size_t read_callback(char *buffer, size_t size, size_t nitems, void *userdata)
{
    auto &data = *static_cast<std::span<const char> *>(userdata);
    const auto copy_size = std::min<size_t>(size * nitems, data.size());

    std::copy_n(data.begin(), copy_size, buffer);
    data = data.subspan(copy_size);

    return copy_size;
}

/*!
 * @brief キャラクタダンプを引数で指定した出力ストリームに書き込む
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param stream 書き込む出力ストリーム
 * @return エラーコード
 */
static errr make_dump(PlayerType *player_ptr, std::ostream &stream)
{
    FILE *fff;
    GAME_TEXT file_name[1024];

    /* Open a new file */
    fff = angband_fopen_temp(file_name, 1024);
    if (!fff) {
#ifdef JP
        msg_format("一時ファイル %s を作成できませんでした。", file_name);
#else
        msg_format("Failed to create temporary file %s.", file_name);
#endif
        msg_print(nullptr);
        return 1;
    }

    /* 一旦一時ファイルを作る。通常のダンプ出力と共通化するため。 */
    make_character_dump(player_ptr, fff);
    angband_fclose(fff);

    // 一時ファイルを削除する前に閉じるためブロックにする
    {
        std::ifstream ifs(file_name);
        stream << ifs.rdbuf();
    }

    fd_kill(file_name);

    /* Success */
    return 0;
}

/*!
 * @brief スクリーンダンプを作成する/ Make screen dump to buffer
 * @return 作成したスクリーンダンプの参照ポインタ
 */
std::string make_screen_dump(PlayerType *player_ptr)
{
    constexpr auto html_head =
        "<html>\n<body text=\"#ffffff\" bgcolor=\"#000000\">\n"
        "<pre>\n";
    constexpr auto html_foot =
        "</pre>\n"
        "</body>\n</html>\n";

    const auto &[wid, hgt] = term_get_size();
    std::stringstream screen_ss;

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    bool old_use_graphics = use_graphics;
    if (old_use_graphics) {
        /* Clear -more- prompt first */
        msg_print(nullptr);

        use_graphics = false;
        reset_visuals(player_ptr);

        static constexpr auto flags = {
            MainWindowRedrawingFlag::WIPE,
            MainWindowRedrawingFlag::BASIC,
            MainWindowRedrawingFlag::EXTRA,
            MainWindowRedrawingFlag::MAP,
            MainWindowRedrawingFlag::EQUIPPY,
        };
        rfu.set_flags(flags);
        handle_stuff(player_ptr);
    }

    screen_ss << html_head;

    /* Dump the screen */
    for (int y = 0; y < hgt; y++) {
        /* Start the row */
        if (y != 0) {
            screen_ss << '\n';
        }

        /* Dump each row */
        uint8_t old_a = 0;
        DisplaySymbol ds(0, ' ');
        for (int x = 0; x < wid - 1; x++) {
            int rv, gv, bv;
            concptr cc = nullptr;
            ds = term_what(x, y, ds);

            switch (ds.character) {
            case '&':
                cc = "&amp;";
                break;
            case '<':
                cc = "&lt;";
                break;
            case '>':
                cc = "&gt;";
                break;
            case '"':
                cc = "&quot;";
                break;
            case '\'':
                cc = "&#39;";
                break;
#ifdef WINDOWS
            case 0x1f:
                ds.character = '.';
                break;
            case 0x7f:
                ds.character = (ds.color == 0x09) ? '%' : '#';
                break;
#endif
            }

            ds.color = ds.color & 0x0F;
            if (((y == 0) && (x == 0)) || (ds.color != old_a)) {
                rv = angband_color_table[ds.color][1];
                gv = angband_color_table[ds.color][2];
                bv = angband_color_table[ds.color][3];
                screen_ss << format("%s<font color=\"#%02x%02x%02x\">", ((y == 0 && x == 0) ? "" : "</font>"), rv, gv, bv);
                old_a = ds.color;
            }

            if (cc) {
                screen_ss << cc;
            } else {
                screen_ss << ds.character;
            }
        }
    }

    screen_ss << "</font>\n";

    screen_ss << html_foot;

    std::string ret;
    if (const auto screen_dump_size = screen_ss.tellp();
        (0 <= screen_dump_size) && (screen_dump_size < SCREEN_BUF_MAX_SIZE)) {
        ret = screen_ss.str();
    }

    if (!old_use_graphics) {
        return ret;
    }

    use_graphics = true;
    reset_visuals(player_ptr);
    static constexpr auto flags = {
        MainWindowRedrawingFlag::WIPE,
        MainWindowRedrawingFlag::BASIC,
        MainWindowRedrawingFlag::EXTRA,
        MainWindowRedrawingFlag::MAP,
        MainWindowRedrawingFlag::EQUIPPY,
    };
    rfu.set_flags(flags);
    handle_stuff(player_ptr);
    return ret;
}

/*!
 * @brief スコア転送処理のメインルーチン
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 正常にスコアを送信できたらtrue、失敗時に送信を中止したらfalse
 */
bool report_score(PlayerType *player_ptr)
{
    std::stringstream score_ss;
    std::string personality_desc = ap_ptr->title.string();
    personality_desc.append(_(ap_ptr->no ? "の" : "", " "));

    PlayerRealm pr(player_ptr);
    const auto &realm1_name = PlayerClass(player_ptr).equals(PlayerClassType::ELEMENTALIST) ? get_element_title(player_ptr->element) : pr.realm1().get_name().string();
    score_ss << fmt::format("name: {}\n", player_ptr->name)
             << fmt::format("version: {}\n", AngbandSystem::get_instance().build_version_expression(VersionExpression::FULL))
             << fmt::format("score: {}\n", calc_score(player_ptr))
             << fmt::format("level: {}\n", player_ptr->lev)
             << fmt::format("depth: {}\n", player_ptr->current_floor_ptr->dun_level)
             << fmt::format("maxlv: {}\n", player_ptr->max_plv)
             << fmt::format("maxdp: {}\n", DungeonRecords::get_instance().get_record(DungeonId::ANGBAND).get_max_level())
             << fmt::format("au: {}\n", player_ptr->au);
    const auto &igd = InnerGameData::get_instance();
    score_ss << fmt::format("turns: {}\n", igd.get_real_turns(AngbandWorld::get_instance().game_turn))
             << fmt::format("sex: {}\n", enum2i(player_ptr->psex))
             << fmt::format("race: {}\n", rp_ptr->title)
             << fmt::format("class: {}\n", cp_ptr->title)
             << fmt::format("seikaku: {}\n", personality_desc)
             << fmt::format("realm1: {}\n", realm1_name)
             << fmt::format("realm2: {}\n", pr.realm2().get_name())
             << fmt::format("killer: {}\n", player_ptr->died_from)
             << "-----charcter dump-----\n";

    make_dump(player_ptr, score_ss);
    if (!screen_dump.empty()) {
        score_ss << "-----screen shot-----\n"
                 << screen_dump;
    }

    term_clear();
    while (true) {
        term_fresh();
        prt(_("スコア送信中...", "Sending the score..."), 0, 0);
        term_fresh();

        prt(_("スコア・サーバへの送信に失敗しました。", "Failed to send to the score server."), 0, 0);
        (void)inkey();
        if (input_check_strict(player_ptr, _("もう一度接続を試みますか? ", "Try again? "), UserCheck::NO_HISTORY)) {
            continue;
        }

        return false;
    }
}
#else
#endif /* WORLD_SCORE */
