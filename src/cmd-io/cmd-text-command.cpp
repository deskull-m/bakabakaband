/*!
 * @file cmd-text-command.cpp
 * @brief 文章コマンド処理の実装
 */

#include "cmd-io/cmd-text-command.h"
#include "cmd-action/cmd-others.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"

#include <algorithm>
#include <cctype>
#include <functional>
#include <string>
#include <unordered_map>

// コマンドマッピング用の構造体
struct TextCommand {
    std::vector<std::string> keywords;
    std::function<void(PlayerType *)> action;
    std::string description;
};

/*!
 * @brief 文字列を小文字に変換し、空白を除去する
 */
static std::string normalize_string(const std::string &str)
{
    std::string result;
    for (char c : str) {
        if (!std::isspace(c)) {
            result += std::tolower(c);
        }
    }
    return result;
}

/*!
 * @brief 入力文字列がキーワードにマッチするかチェック
 */
static bool matches_keywords(const std::string &input, const std::vector<std::string> &keywords)
{
    std::string normalized_input = normalize_string(input);

    for (const auto &keyword : keywords) {
        std::string normalized_keyword = normalize_string(keyword);
        if (normalized_input.find(normalized_keyword) != std::string::npos) {
            return true;
        }
    }
    return false;
}

/*!
 * @brief 文章コマンドのマッピングテーブルを取得
 */
static std::vector<TextCommand> get_text_commands()
{
    return {
        { { "search", "探す", "探索" },
            [](PlayerType *player_ptr) {
                do_cmd_search(player_ptr);
            },
            _("周囲を探索する", "Search") },
        { { "suicide", "死ぬ", "自殺" },
            [](PlayerType *player_ptr) {
                do_cmd_suicide(player_ptr);
            },
            _("自殺する", "suicide") }
    };
}

/*!
 * @brief 文章コマンド入力処理
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_text_command(PlayerType *player_ptr)
{
    screen_save();

    msg_erase();

    // 入力ループ
    tl::optional<std::string> buf;
    while (true) {
        buf = input_string(_("コマンド: ", "COMMAND: "), 1024);
        if (!buf.has_value()) {
            return;
        }
        if (buf->empty()) {
            continue;
        }
        break;
    }

    screen_load();

    // コマンドを検索して実行
    auto commands = get_text_commands();
    bool found = false;

    for (const auto &cmd : commands) {
        if (matches_keywords(buf.value(), cmd.keywords)) {
            cmd.action(player_ptr);
            found = true;
            break;
        }
    }

    if (!found) {
        msg_format(_("'%s' は認識できないコマンドです。", "'%s' is not a recognized command."), buf.value().c_str());
        msg_print(_("'ヘルプ' と入力すると利用可能なコマンドが表示されます。", "Type 'help' to see available commands."));
    }
}
