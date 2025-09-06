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
#include "player-status/player-energy.h"
#include "spell-kind/spells-sight.h"
#include "spell/spells-status.h"
#include "status/buff-setter.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "util/probability-table.h"
#include "view/display-messages.h"

#include <algorithm>
#include <cctype>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

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
            _("自殺する", "suicide") },
        { { "dance", "踊る", "ダンス", "おどる" },
            [](PlayerType *player_ptr) {
                // 踊るアクション
                msg_print(_("あなたは楽しそうに踊り始めた！", "You start dancing joyfully!"));

                // ランダムな踊りメッセージ
                std::vector<std::string> dance_messages_jp = {
                    "あなたは優雅にワルツを踊った。",
                    "あなたは情熱的にタンゴを踊った。",
                    "あなたは軽やかにスキップした。",
                    "あなたは力強くステップを踏んだ。",
                    "あなたはくるくると回転した。",
                    "あなたは陽気にジグを踊った。"
                };

                std::vector<std::string> dance_messages_en = {
                    "You dance a graceful waltz.",
                    "You dance a passionate tango.",
                    "You skip lightly.",
                    "You take powerful steps.",
                    "You spin around and around.",
                    "You dance a merry jig."
                };

                int choice = randint0(dance_messages_jp.size());

#ifdef JP
                msg_print(dance_messages_jp[choice].c_str());
#else
                msg_print(dance_messages_en[choice].c_str());
#endif

                // 少しの体力消費
                if (player_ptr->csp > 1) {
                    player_ptr->csp--;
                    auto &rfu = RedrawingFlagsUpdater::get_instance();
                    rfu.set_flag(MainWindowRedrawingFlag::MP);
                }

                // 時間消費
                PlayerEnergy(player_ptr).set_player_turn_energy(50);

                // 周囲のモンスターが反応する可能性
                if (one_in_(3)) {
                    msg_print(_("あなたの踊りに周囲が注目している。", "Your dancing attracts attention."));
                    aggravate_monsters(player_ptr, 0);
                }

                // 稀にポジティブ効果
                if (one_in_(10)) {
                    msg_print(_("素晴らしい踊りで気分が高揚した！", "Your wonderful dance lifts your spirits!"));
                    set_hero(player_ptr, player_ptr->blessed + randint1(10), false);
                }
            },
            _("踊る", "Dance") }
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
