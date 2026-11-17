/*!
 * @file report.c
 * @brief スコアサーバ転送機能の実装
 * @date 2014/07/14
 * @author Bakabakaband Team
 */

#include "io/report.h"
#include "core/asking-player.h"
#include "core/visuals-reseter.h"
#include "game-option/special-options.h"
#include "io-dump/character-dump.h"
#include "io/input-context.h"
#include "io/score-submission.h"
#include "locale/character-encoding.h"
#include "net/http-client.h"
#include "system/angband-system.h"
#include "system/creature-entity.h"
#include "system/system-variables.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "util/angband-files.h"
#include "view/display-messages.h"
#include "view/display-symbol.h"
#include <cstdlib>
#include <fmt/format.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>

#ifdef WORLD_SCORE

/*!
 * @brief スコアサーバの送信先 URL (既定値)
 * @details 環境変数 BAKABAKABAND_SCORE_URL で上書きできる。
 * サーバ側の実装は https://github.com/deskull-m/bakabakaband-score を参照。
 */
constexpr auto DEFAULT_SCORE_SERVER_URL = "http://localhost:3000/submit";

/*!
 * @brief スコアサーバの送信先 URL を返す
 */
static std::string get_score_server_url()
{
    const auto *env = std::getenv("BAKABAKABAND_SCORE_URL");
    if (env != nullptr && env[0] != '\0') {
        return env;
    }

    return DEFAULT_SCORE_SERVER_URL;
}

/*!
 * @brief キャラクタダンプを引数で指定した出力ストリームに書き込む
 * @param creature クリーチャーへの参照
 * @param stream 書き込む出力ストリーム
 * @return エラーコード
 */
static errr make_dump(CreatureEntity &creature, std::ostream &stream)
{
    FILE *fff;
    GAME_TEXT file_name[1024];

    /* Open a new file */
    fff = angband_fopen_temp(file_name, 1024);
    if (!fff) {
#ifdef JP
        msg_print("一時ファイル {} を作成できませんでした。", file_name);
#else
        msg_print("Failed to create temporary file {}.", file_name);
#endif
        msg_erase();
        return 1;
    }

    /* 一旦一時ファイルを作る。通常のダンプ出力と共通化するため。 */
    make_character_dump(creature, fff);
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

namespace {

constexpr auto SCREEN_FORMAT = "bakabakaband-screen";
constexpr auto SCREEN_FORMAT_VERSION = 1;
constexpr auto SCREEN_PALETTE_SIZE = 16;

nlohmann::json screen_dump;

/*!
 * @brief システム文字コードの文字列を UTF-8 にして返す。変換できなければそのまま返す
 */
std::string screen_text_to_utf8(std::string_view text)
{
    const auto converted = sys_to_utf8(text);
    return converted ? *converted : std::string(text);
}

/*!
 * @brief 1 行分のセル列を「色番号と文字列」の連続区間 (run) の配列にする
 * @details 行末の空白だけの区間は落とす。表示側は width まで空白で埋めて描画する。
 */
nlohmann::json make_screen_row(int y, int width)
{
    nlohmann::json runs = nlohmann::json::array();
    std::string text;
    uint8_t run_color = 0;
    auto push_run = [&](bool is_last) {
        if (is_last) {
            const auto end = text.find_last_not_of(' ');
            text.erase(end == std::string::npos ? 0 : end + 1);
        }

        if (!text.empty()) {
            runs.push_back({ run_color, screen_text_to_utf8(text) });
        }

        text.clear();
    };

    DisplaySymbol ds(0, ' ');
    for (auto x = 0; x < width; x++) {
        ds = term_what(x, y, ds);
#ifdef WINDOWS
        switch (ds.character) {
        case 0x1f:
            ds.character = '.';
            break;
        case 0x7f:
            ds.character = (ds.color == 0x09) ? '%' : '#';
            break;
        default:
            break;
        }
#endif
        const auto color = static_cast<uint8_t>(ds.color & 0x0F);
        if (x > 0 && color != run_color) {
            push_run(false);
        }

        run_color = color;
        text.push_back(ds.character);
    }

    push_run(true);
    return runs;
}
}

/*!
 * @brief 画面の内容を色付きテキストの JSON として取得する
 * @param creature クリーチャーへの参照
 * @return bakabakaband-screen 形式の JSON オブジェクト。大きすぎる場合は null
 * @details
 * {
 *   "format": "bakabakaband-screen", "version": 1,
 *   "width": 80, "height": 24,
 *   "palette": ["#000000", "#ffffff", ...],      // 色番号 0-15 の RGB
 *   "rows": [ [[1, "@..#"], [7, "..."]], ... ]   // 行ごとに [色番号, 文字列] の連続区間
 * }
 * 描画は閲覧側 (スコアサーバ) が行うので、ここでは HTML を組み立てない。
 */
nlohmann::json make_screen_dump(CreatureEntity &creature)
{
    const auto &[wid, hgt] = term_get_size();
    if (use_graphics) {
        /* Clear -more- prompt first */
        msg_erase();
    }

    const GraphicsSuspender graphics_suspender(creature);

    nlohmann::json palette = nlohmann::json::array();
    for (auto i = 0; i < SCREEN_PALETTE_SIZE; i++) {
        palette.push_back(fmt::format("#{:02x}{:02x}{:02x}", angband_color_table[i][1], angband_color_table[i][2], angband_color_table[i][3]));
    }

    nlohmann::json rows = nlohmann::json::array();
    for (auto y = 0; y < hgt; y++) {
        rows.push_back(make_screen_row(y, wid));
    }

    nlohmann::json screen = {
        { "format", SCREEN_FORMAT },
        { "version", SCREEN_FORMAT_VERSION },
        { "width", wid },
        { "height", hgt },
        { "palette", palette },
        { "rows", rows },
    };
    if (screen.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace).size() >= SCREEN_BUF_MAX_SIZE) {
        return nullptr;
    }

    return screen;
}

void capture_screen_dump(CreatureEntity &creature)
{
    screen_dump = make_screen_dump(creature);
}

const nlohmann::json &get_screen_dump()
{
    return screen_dump;
}

/*!
 * @brief サーバの応答から表示用のメッセージを取り出す
 * @param body 応答本文 (JSON を期待するが、そうでなくてもよい)
 * @return 応答に含まれる error または message。無ければ空文字列
 */
static std::string extract_response_message(const std::string &body)
{
    const auto json = nlohmann::json::parse(body, nullptr, false);
    if (json.is_discarded() || !json.is_object()) {
        return "";
    }

    for (const auto *key : { "error", "message" }) {
        if (json.contains(key) && json[key].is_string()) {
            return json[key].get<std::string>();
        }
    }

    return "";
}

/*!
 * @brief スコア転送処理のメインルーチン
 * @param creature クリーチャーへの参照
 * @return 正常にスコアを送信できたらtrue、失敗時に送信を中止したらfalse
 * @details
 * bakabakaband-score-submission 形式の JSON を組み立てて HTTP POST する。
 * 送信内容 (submission_id を含む) は最初に一度だけ組み立て、再試行でも同じものを送る。
 * サーバは submission_id と内容で重複登録を抑止するので、何度送っても 1 件にしかならない。
 */
bool report_score(CreatureEntity &creature)
{
    std::stringstream dump_ss;
    make_dump(creature, dump_ss);
    const auto submission = make_score_submission(creature, dump_ss.str(), screen_dump);
    const auto body = submission.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
    const auto url = get_score_server_url();

    http::Client client;
    client.user_agent = "Bakabakaband/" + AngbandSystem::get_instance().build_version_expression(VersionExpression::WITH_EXTRA);

    term_clear();
    while (true) {
        term_fresh();
        prt(_("スコア送信中...", "Sending the score..."), 0, 0);
        term_fresh();

        const auto response = client.post(url, body, "application/json");
        if (response && response->status == 200) {
            return true;
        }

        std::string reason;
        if (!response) {
            reason = _("接続できませんでした", "could not connect");
        } else {
            const auto message = extract_response_message(response->body);
            reason = fmt::format("HTTP {}{}{}", response->status, message.empty() ? "" : ": ", message);
        }

        const auto prompt = fmt::format(_("スコア・サーバへの送信に失敗しました。({})", "Failed to send to the score server. ({})"), reason);
        prt(prompt, 0, 0);
        (void)input_key_with_context(InputKind::ANY_KEY, prompt);
        if (input_check_strict(creature, _("もう一度接続を試みますか? ", "Try again? "), UserCheck::NO_HISTORY)) {
            continue;
        }

        return false;
    }
}
#endif /* WORLD_SCORE */
