/*!
 * @brief シンボルの一覧の左端の文字を求める処理のテスト
 */

#include "io-dump/dump-util.h"
#include "test/scoped-restore.h"
#include "view/display-symbol.h"
#include <doctest/doctest.h>

TEST_CASE("calc_visual_list_char_left starts the list 10 characters before the selected one")
{
    CHECK(calc_visual_list_char_left('.') == 0x24);
    CHECK(calc_visual_list_char_left('\x05') == 0);
    CHECK(calc_visual_list_char_left('\x0a') == 0);
    CHECK(calc_visual_list_char_left('\x7f') == 0x75);
}

TEST_CASE("calc_visual_list_char_left treats graphic tiles as unsigned characters")
{
    // 0x80 以上の文字は char が符号付きの環境では負になるが、符号なしの文字として 10 文字手前を返す
    CHECK(calc_visual_list_char_left('\x80') == 0x76);
    CHECK(calc_visual_list_char_left('\x89') == 0x7f);
    CHECK(calc_visual_list_char_left('\xa2') == 0x98);
    CHECK(calc_visual_list_char_left('\xff') == 0xf5);
}

TEST_CASE("visual_mode_command starts the visual list near a graphic tile")
{
    auto &clipboard = DisplaySymbolsClipboard::get_instance();
    const auto restore_clipboard = test::scoped_restore(clipboard.symbol, clipboard.symbols);
    auto visual_list = false;
    TERM_COLOR attr_top = 0;
    byte char_left = 0;
    TERM_COLOR attr = 0x85;
    auto character = '\xa2';
    auto need_redraw = false;

    SUBCASE("v opens the list around the current tile")
    {
        CHECK(visual_mode_command('v', &visual_list, 15, 69, &attr_top, &char_left, &attr, &character, &need_redraw));
        CHECK(visual_list);
        CHECK(char_left == 0x98);
    }

    SUBCASE("p moves the list to the pasted tile")
    {
        visual_list = true;
        clipboard.symbol = { 0x86, '\xc8' };
        CHECK(visual_mode_command('p', &visual_list, 15, 69, &attr_top, &char_left, &attr, &character, &need_redraw));
        CHECK(static_cast<uint8_t>(character) == 0xc8);
        CHECK(char_left == 0xbe);
    }
}
