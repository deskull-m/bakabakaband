#include "birth/birth-select-race.h"
#include "birth/birth-util.h"
#include "io/input-key-acceptor.h"
#include "player-info/race-info.h"
#include "player/race-info-table.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "util/enum-converter.h"
#include "util/int-char-converter.h"
#include <sstream>
#include <vector>

static std::string birth_race_label(int cs, concptr sym)
{
    const char p2 = ')';
    std::stringstream ss;

    if (cs < 0 || cs >= MAX_RACES) {
        ss << '*' << p2 << _("ランダム", "Random");
    } else {
        ss << sym[cs] << p2 << race_info[cs].title;
    }
    return ss.str();
}

static void enumerate_race_list(PlayerType *player_ptr, char *sym)
{
    int display_index = 0;
    for (int n = 0; n < MAX_RACES; n++) {
        if (!race_info[n].playable) {
            continue;
        }

        player_ptr->race = &race_info[n];
        if (display_index < 26) {
            sym[n] = I2A(display_index);
        } else {
            sym[n] = ('A' + display_index - 26);
        }

        put_str(birth_race_label(n, sym), 12 + (display_index / 5), 1 + 16 * (display_index % 5));
        display_index++;
    }
}

static int get_random_playable_race()
{
    std::vector<int> playable_races;
    for (int i = 0; i < MAX_RACES; i++) {
        if (race_info[i].playable) {
            playable_races.push_back(i);
        }
    }
    return playable_races[randint0(playable_races.size())];
}

static std::string display_race_stat(PlayerType *player_ptr, int cs, int *os, const std::string &cur, concptr sym)
{
    if (cs == *os) {
        return cur;
    }

    c_put_str(TERM_WHITE, cur, 12 + (*os / 5), 1 + 16 * (*os % 5));
    put_str("                                   ", 3, 40);
    auto result = birth_race_label(cs, sym);
    if (cs == MAX_RACES) {
        put_str("                                   ", 4, 40);
        put_str("                                   ", 5, 40);
        put_str("                                   ", 6, 40);
    } else {
        player_ptr->race = &race_info[cs];
        c_put_str(TERM_L_BLUE, player_ptr->race->title, 3, 40);
        put_str(_("腕力 知能 賢さ 器用 耐久 魅力 経験 ", "Str  Int  Wis  Dex  Con  Chr   EXP "), 4, 40);
        put_str(_("の種族修正", ": Race modification"), 3, 40 + player_ptr->race->title->length());

        const auto stats = format("%+3d  %+3d  %+3d  %+3d  %+3d  %+3d %+4d%% ", player_ptr->race->r_adj[0], player_ptr->race->r_adj[1], player_ptr->race->r_adj[2], player_ptr->race->r_adj[3], player_ptr->race->r_adj[4], player_ptr->race->r_adj[5], (player_ptr->race->r_exp - 100));
        c_put_str(TERM_L_BLUE, stats, 5, 40);

        put_str("HD ", 6, 40);
        const auto hd = format("%2d", player_ptr->race->r_mhp);
        c_put_str(TERM_L_BLUE, hd, 6, 43);

        put_str(_("隠密", "Stealth"), 6, 47);
        const auto stealth = format("%+2d", player_ptr->race->r_stl);
        c_put_str(TERM_L_BLUE, stealth, 6, _(52, 55));

        put_str(_("赤外線視力", "Infra"), 6, _(56, 59));
        const auto infra = format(_("%%2dft", "%%2dft"), 10 * player_ptr->race->infra);
        c_put_str(TERM_L_BLUE, infra, 6, _(67, 65));
    }

    c_put_str(TERM_YELLOW, result, 12 + (cs / 5), 1 + 16 * (cs % 5));
    *os = cs;
    return result;
}

static void interpret_race_select_key_move(char c, int *cs)
{
    if (c == '8') {
        if (*cs >= 5) {
            *cs -= 5;
        }
    }

    if (c == '4') {
        if (*cs > 0) {
            (*cs)--;
        }
    }

    if (c == '6') {
        if (*cs < MAX_RACES) {
            (*cs)++;
        }
    }

    if (c == '2') {
        if ((*cs + 5) <= MAX_RACES) {
            *cs += 5;
        }
    }
}

static bool select_race(PlayerType *player_ptr, char *sym, int *k)
{
    auto cs = enum2i(player_ptr->prace);
    int os = MAX_RACES;
    std::string cur = birth_race_label(os, sym);
    while (true) {
        cur = display_race_stat(player_ptr, cs, &os, cur, sym);
        if (*k >= 0) {
            break;
        }

        const auto buf = format(_("種族を選んで下さい (%c-%c) ('='初期オプション設定): ", "Choose a race (%c-%c) ('=' for options): "), sym[0], sym[MAX_RACES - 1]);
        put_str(buf, 10, 10);
        char c = inkey();
        if (c == 'Q') {
            birth_quit();
        }

        if (c == 'S') {
            return false;
        }

        if (c == ' ' || c == '\r' || c == '\n') {
            if (cs == MAX_RACES) {
                *k = get_random_playable_race();
                cs = *k;
                continue;
            } else {
                *k = cs;
                break;
            }
        }

        if (c == '*') {
            *k = get_random_playable_race();
            cs = *k;
            continue;
        }

        interpret_race_select_key_move(c, &cs);
        *k = (islower(c) ? A2I(c) : -1);
        if ((*k >= 0) && (*k < MAX_RACES) && race_info[*k].playable) {
            cs = *k;
            continue;
        }

        *k = (isupper(c) ? (26 + c - 'A') : -1);
        if ((*k >= 26) && (*k < MAX_RACES) && race_info[*k].playable) {
            cs = *k;
            continue;
        } else {
            *k = -1;
        }

        birth_help_option(player_ptr, c, BirthKind::RACE);
    }

    return true;
}

/*!
 * @brief プレイヤーの種族選択を行う / Player race
 */
bool get_player_race(PlayerType *player_ptr)
{
    clear_from(10);
    put_str(
        _("注意：《種族》によってキャラクターの先天的な資質やボーナスが変化します。", "Note: Your 'race' determines various intrinsic factors and bonuses."),
        23, 5);

    char sym[MAX_RACES];
    enumerate_race_list(player_ptr, sym);
    int k = -1;
    if (!select_race(player_ptr, sym, &k)) {
        return false;
    }

    player_ptr->prace = i2enum<PlayerRaceType>(k);
    player_ptr->race = &race_info[k];
    c_put_str(TERM_L_BLUE, player_ptr->race->title, 4, 15);
    return true;
}
