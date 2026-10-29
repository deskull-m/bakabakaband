#include "term/screen-processor.h"
#include "io/input-key-acceptor.h"
#include "locale/japanese.h"
#include "term/term-color-types.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "view/display-symbol.h"
#include "world/world.h"

/*
 * Hack -- prevent "accidents" in "screen_save()" or "screen_load()"
 */
static int screen_depth = 0;

/*
 * Move the cursor
 */
void move_cursor(int row, int col)
{
    term_gotoxy(col, row);
}

/*
 * Flush all input chars.  Actually, remember the flush,
 * and do a "special flush" before the next "inkey()".
 *
 * This is not only more efficient, but also necessary to make sure
 * that various "inkey()" codes are not "lost" along the way.
 */
void flush(void)
{
    inkey_xtra = true;
}

/*
 * Save the screen, and increase the "icky" depth.
 *
 * This function must match exactly one call to "screen_load()".
 */
void screen_save()
{
    msg_erase();

    term_save();

    AngbandWorld::get_instance().character_icky_depth++;
    screen_depth++;
}

/*
 * Load the screen, and decrease the "icky" depth.
 *
 * This function must match exactly one call to "screen_save()".
 */
void screen_load(ScreenLoadOptType opt)
{
    msg_erase();
    auto &world = AngbandWorld::get_instance();
    switch (opt) {
    case ScreenLoadOptType::ONE:
        term_load(false);
        world.character_icky_depth--;
        screen_depth--;
        break;
    case ScreenLoadOptType::ALL:
        term_load(true);
        world.character_icky_depth -= static_cast<byte>(screen_depth);
        screen_depth = 0;
        break;
    default:
        break;
    }
}

/*
 * Display a string on the screen using an attribute.
 *
 * At the given location, using the given attribute, if allowed,
 * add the given string.  Do not clear the line.
 */
void c_put_str(TERM_COLOR attr, std::string_view sv, TERM_LEN row, TERM_LEN col)
{
    term_putstr(col, row, -1, attr, sv);
}

/*
 * As above, but in "white"
 */
void put_str(std::string_view sv, TERM_LEN row, TERM_LEN col)
{
    term_putstr(col, row, -1, TERM_WHITE, sv);
}

/*
 * Display a string on the screen using an attribute, and clear
 * to the end of the line.
 */
void c_prt(TERM_COLOR attr, std::string_view sv, TERM_LEN row, TERM_LEN col)
{
    term_erase(col, row);
    term_addstr(-1, attr, sv);
}

/*
 * As above, but in "white"
 */
void prt(std::string_view sv, TERM_LEN row, TERM_LEN col)
{
    /* Spawn */
    c_prt(TERM_WHITE, sv, row, col);
}

/*!
 * @brief 画面に表示済みの行の末尾 (x の手前) の 1 文字のマスを取り出す
 * @return 末尾の文字のマス (全角なら 2 マス、半角なら 1 マス)。x が 0 以下なら空
 * @details 全角文字の後半のマスには、c_roff() が属性に 0x20 の印を付けている。
 */
static std::vector<DisplaySymbol> last_cells_of_line(int x, int y)
{
    std::vector<DisplaySymbol> cells;
    if (x <= 0) {
        return cells;
    }

    const auto is_full_width = (term_what(x - 1, y, {}).color & 0x20) != 0;
    const auto start = is_full_width ? (x - 2) : (x - 1);
    for (auto col = start; col < x; col++) {
        cells.push_back(term_what(col, y, {}));
    }

    return cells;
}

static std::vector<DisplaySymbol> c_roff_wrap(int x, int y, int w, std::string_view ch)
{
    if (x >= w) {
        return {};
    }

    std::vector<DisplaySymbol> wrap_chars;
    auto wrap_col = w;

    // 行の末尾 (x の手前) の文字。行末禁則文字 (始め括弧) を行末に残さないための判定に使う。
    const auto last_cells = last_cells_of_line(x, y);
    std::string last_char;
    for (const auto &cell : last_cells) {
        last_char.push_back(cell.character);
    }
    const auto last_is_full_width = last_cells.size() == 2;
    const auto line_ends_with_kinsoku = is_line_end_kinsoku(last_char);

    // 次の行の先頭が行頭禁則文字になるとき、または行末が行末禁則文字になるときは、行末の１文字を次の行へ送る。
    // 半角の始め括弧は下の語単位の折り返しがそのまま次の行へ送るので、全角のときだけここで送る。
    const auto wraps_at_last_char = (((ch.length() == 2) && is_kinsoku(ch)) || line_ends_with_kinsoku) && ((ch.length() == 2) || last_is_full_width);
    if (wraps_at_last_char) {
        wrap_chars = last_cells;
        wrap_col = x - static_cast<int>(last_cells.size());
    } else if (ch.length() == 1) {
        /* 現在が半角文字の場合 */
        for (auto i = 0; i < x; i++) {
            DisplaySymbol ds;
            ds = term_what(i, y, ds);

            if (ds.character == ' ') {
                wrap_col = i + 1;
                wrap_chars.clear();
            } else if (_(iskanji(ds.character), false)) {
                wrap_col = i + 2;
                i++;
                wrap_chars.clear();
            } else {
                wrap_chars.push_back(ds);
            }
        }
    }

    term_erase(wrap_col, y);
    return wrap_chars;
}

/*
 * Print some (colored) text to the screen at the current cursor position,
 * automatically "wrapping" existing text (at spaces) when necessary to
 * avoid placing any text into the last column, and clearing every line
 * before placing any text in that line.  Also, allow "newline" to force
 * a "wrap" to the next line.  Advance the cursor as needed so sequential
 * calls to this function will work correctly.
 *
 * Once this function has been called, the cursor should not be moved
 * until all the related "c_roff()" calls to the window are complete.
 *
 * This function will correctly handle any width up to the maximum legal
 * value of 256, though it works best for a standard 80 character width.
 */
void c_roff(TERM_COLOR a, std::string_view str)
{
    const auto &[wid, hgt] = term_get_size();
    auto [x, y] = term_locate();
    if (y == hgt - 1 && x > wid - 3) {
        return;
    }

    for (auto s = str.begin(); s != str.end(); ++s) {
        const auto is_kanji = is_multibyte_char_at(std::string_view(s, str.end()));

        if (*s == '\n') {
            if (y + 1 < hgt) {
                term_erase(0, y + 1);
            }

            return;
        }

        const auto ch = (is_kanji || isprint(*s)) ? *s : ' ';

        if ((x >= ((is_kanji) ? wid - 2 : wid - 1)) && (ch != ' ')) {
            const auto wrap_chars = c_roff_wrap(x, y, wid, std::string_view(s, std::next(s, is_kanji ? 2 : 1)));

            y++;
            if (y == hgt) {
                return;
            }

            term_erase(0, y);
            for (const auto &symbol : wrap_chars) {
                term_addch(symbol);
            }
            x = wrap_chars.size();
        }

        term_addch({ static_cast<uint8_t>(_(a | 0x10, a)), ch });
        if (is_kanji) {
            s++;
            x++;
            term_addch({ static_cast<uint8_t>(a | 0x20), *s });
        }

        if (++x > wid) {
            x = wid;
        }
    }
}

/*
 * As above, but in "white"
 */
void roff(std::string_view str)
{
    /* Spawn */
    c_roff(TERM_WHITE, str);
}

/*
 * Clear part of the screen
 */
void clear_from(int row)
{
    for (int y = row; y < game_term->hgt; y++) {
        TermOffsetSetter tos(0, tl::nullopt);
        term_erase(0, y);
    }
}
