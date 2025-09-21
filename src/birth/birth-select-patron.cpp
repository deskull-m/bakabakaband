#include "birth/birth-select-patron.h"
#include "birth/birth-util.h"
#include "io/input-key-acceptor.h"
#include "player/patron.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "util/int-char-converter.h"
#include "view/display-player-misc-info.h"
#include <sstream>

static std::string birth_patron_label(int cs, concptr sym)
{
    const char p2 = ')';
    std::stringstream ss;

    if (cs < 0 || cs >= MAX_PATRON) {
        ss << '*' << p2 << _("ランダム", "Random");
    } else {
        ss << sym[cs] << p2 << patron_list[cs].name.string();
    }
    return ss.str();
}

static void enumerate_patron_list(char *sym)
{
    for (int n = 0; n < MAX_PATRON; n++) {
        if (n < 26) {
            sym[n] = I2A(n);
        } else {
            sym[n] = ('A' + n - 26);
        }

        put_str(birth_patron_label(n, sym), 12 + (n / 4), 2 + 18 * (n % 4));
    }
}

static std::string display_patron_stat(int cs, int *os, const std::string &cur, concptr sym)
{
    if (cs == *os) {
        return cur;
    }

    c_put_str(TERM_WHITE, cur, 12 + (*os / 4), 2 + 18 * (*os % 4));
    put_str("                                   ", 3, 40);
    auto result = birth_patron_label(cs, sym);
    if (cs == MAX_PATRON) {
        put_str("                                   ", 4, 40);
        put_str("                                   ", 5, 40);
        put_str("                                   ", 6, 40);
    } else {
        c_put_str(TERM_L_BLUE, patron_list[cs].name.string(), 3, 40);
        put_str(_("のパトロン", ": Patron"), 3, 40 + patron_list[cs].name.string().length());
        put_str(_("パトロンの加護を受ける", "You serve this patron"), 4, 40);
    }

    c_put_str(TERM_YELLOW, result, 12 + (cs / 4), 2 + 18 * (cs % 4));
    *os = cs;
    return result;
}

static int interpret_patron_select_key_move(char key, int initial_patron)
{
    auto pp_idx = initial_patron;
    switch (key) {
    case '8':
        if (pp_idx >= 4) {
            pp_idx -= 4;
        }
        return pp_idx;
    case '4':
        if (pp_idx > 0) {
            (pp_idx)--;
        }
        return pp_idx;
    case '6':
        if (pp_idx < MAX_PATRON) {
            (pp_idx)++;
        }
        return pp_idx;
    case '2':
        if ((pp_idx + 4) <= MAX_PATRON) {
            pp_idx += 4;
        }
        return pp_idx;
    default:
        return pp_idx;
    }
}

static bool select_patron(PlayerType *player_ptr, int *k, concptr sym)
{
    int cs = player_ptr->patron;
    int os = MAX_PATRON;
    std::string cur = birth_patron_label(os, sym);
    while (true) {
        cur = display_patron_stat(cs, &os, cur, sym);
        if (*k >= 0) {
            break;
        }

        const auto buf = format(_("パトロンを選んで下さい (%c-%c) ('='初期オプション設定): ", "Choose a patron (%c-%c) ('=' for options): "), sym[0], sym[MAX_PATRON - 1]);
        put_str(buf, 10, 10);
        char c = inkey();
        if (c == 'Q') {
            birth_quit();
        }

        if (c == 'S') {
            return false;
        }

        if (c == ' ' || c == '\r' || c == '\n') {
            if (cs == MAX_PATRON) {
                *k = randint0(MAX_PATRON);
                cs = *k;
                continue;
            } else {
                *k = cs;
                break;
            }
        }

        cs = interpret_patron_select_key_move(c, cs);
        if (c == '*') {
            *k = randint0(MAX_PATRON);
            cs = *k;
            continue;
        }

        *k = (islower(c) ? A2I(c) : -1);
        if ((*k >= 0) && (*k < MAX_PATRON)) {
            cs = *k;
            continue;
        }

        *k = (isupper(c) ? (26 + c - 'A') : -1);
        if ((*k >= 26) && (*k < MAX_PATRON)) {
            cs = *k;
            continue;
        } else {
            *k = -1;
        }

        birth_help_option(player_ptr, c, BirthKind::PATRON);
    }

    return true;
}

/*!
 * @brief プレイヤーのパトロン選択を行う / Select player's patron
 */
bool get_player_patron(PlayerType *player_ptr)
{
    clear_from(10);
    put_str(_("注意：《パトロン》によってキャラクターが得る加護が変化します。", "Note: Your patron determines the divine protection you receive."),
        23, 5);
    put_str("                                   ", 6, 40);

    char sym[MAX_PATRON];
    enumerate_patron_list(sym);
    int k = -1;
    if (!select_patron(player_ptr, &k, sym)) {
        return false;
    }

    player_ptr->patron = k;
    display_player_name(player_ptr);
    return true;
}
