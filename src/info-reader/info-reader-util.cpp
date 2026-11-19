#include "info-reader/info-reader-util.h"
#include "util/string-processor.h"

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
