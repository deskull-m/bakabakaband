#include "info-reader/info-reader-util.h"
#include "artifact/random-art-effects.h"
#include "info-reader/parse-error-types.h"
#include "object-enchant/activation-info-table.h"
#include "util/string-processor.h"

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(発動能力用) /
 * Grab one activation index flag
 * @details 発動効果テーブルに登録された名前だけを受け入れる。番号は効果を足すとずれるため、番号での指定は受け付けない。
 * @param what 発動の名前
 * @return 発動能力ID
 */
RandomArtActType grab_one_activation_flag(std::string_view what)
{
    for (const auto &activation : activation_info) {
        if (what == activation.flag) {
            return activation.index;
        }
    }

    // 不明な発動名は NONE を返して呼出側の各リーダーが INVALID_FLAG として扱う
    // (他のリーダー同様、個別メッセージは出さず読込エラーとして集約する)。
    return RandomArtActType::NONE;
}

#ifndef JP
/*!
 * @brief 英語のフレーバーテキストを連結する
 * @details add は両端のスペースを削除し、text が空でなく連結する場合は text の末尾が
 * ".!?" のいずれかならスペースを2つ、それ以外ならスペースを1つ挟んで連結する。
 *
 * @param text 現在までに作成されたフレーバーテキスト
 * @param add 連結するテキスト
 */
void append_english_text(std::string &text, std::string_view add)
{
    const auto add_trimmed = str_trim(add);
    if (add_trimmed.empty()) {
        return;
    }

    if (!text.empty()) {
        constexpr std::string_view eos_symbols = ".!?";
        auto is_eos = eos_symbols.find(text.back()) != std::string_view::npos;
        text.append(is_eos ? "  " : " ");
    }
    text.append(add_trimmed);
}
#endif
