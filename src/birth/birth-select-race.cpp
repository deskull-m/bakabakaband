#include "birth/birth-select-race.h"
#include "birth/birth-util.h"
#include "io/input-key-acceptor.h"
#include "player-info/race-info.h"
#include "player/race-info-table.h"
#include "system/creature-entity.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "util/enum-converter.h"
#include "util/int-char-converter.h"
#include <sstream>
#include <vector>

//!< 選択可能 (playable) な種族の生 (raw) インデックス一覧。
//!< メニューはこの一覧上の「表示インデックス」で操作し、生インデックスとは
//!< 都度この一覧経由で変換する。これにより playable 種族が enum 上で連続して
//!< いなくても (例: 末尾に純血種族を追加した場合) 正しく表示・選択できる。
static std::vector<int> get_playable_race_indices()
{
    std::vector<int> playable_races;
    for (int i = 0; i < MAX_RACES; i++) {
        if (race_info[i].playable) {
            playable_races.push_back(i);
        }
    }
    return playable_races;
}

static std::string birth_race_label(int display_index, concptr sym, const std::vector<int> &playables)
{
    const char p2 = ')';
    std::stringstream ss;

    if (display_index < 0 || display_index >= static_cast<int>(playables.size())) {
        ss << '*' << p2 << _("ランダム", "Random");
    } else {
        ss << sym[display_index] << p2 << race_info[playables[display_index]].title;
    }
    return ss.str();
}

static void enumerate_race_list(CreatureEntity &creature, char *sym, const std::vector<int> &playables)
{
    for (int display_index = 0; display_index < static_cast<int>(playables.size()); display_index++) {
        const auto raw = playables[display_index];
        creature.race = &race_info[raw];
        if (display_index < 26) {
            sym[display_index] = I2A(display_index);
        } else {
            sym[display_index] = ('A' + display_index - 26);
        }

        put_str(birth_race_label(display_index, sym, playables), 12 + (display_index / 5), 1 + 16 * (display_index % 5));
    }
}

static std::string display_race_stat(CreatureEntity &creature, int cs, int *os, const std::string &cur, concptr sym, const std::vector<int> &playables)
{
    if (cs == *os) {
        return cur;
    }

    const auto count = static_cast<int>(playables.size());
    c_put_str(TERM_WHITE, cur, 12 + (*os / 5), 1 + 16 * (*os % 5));
    put_str("                                   ", 3, 40);
    auto result = birth_race_label(cs, sym, playables);
    if (cs == count) {
        put_str("                                   ", 4, 40);
        put_str("                                   ", 5, 40);
        put_str("                                   ", 6, 40);
    } else {
        creature.race = &race_info[playables[cs]];
        c_put_str(TERM_L_BLUE, creature.get_race_info()->title, 3, 40);
        put_str(_("腕力 知能 賢さ 器用 耐久 魅力 経験 ", "Str  Int  Wis  Dex  Con  Chr   EXP "), 4, 40);
        put_str(_("の種族修正", ": Race modification"), 3, 40 + creature.get_race_info()->title->length());

        const auto stats = format("%+3d  %+3d  %+3d  %+3d  %+3d  %+3d %+4d%% ", creature.get_race_info()->r_adj[0], creature.get_race_info()->r_adj[1], creature.get_race_info()->r_adj[2], creature.get_race_info()->r_adj[3], creature.get_race_info()->r_adj[4], creature.get_race_info()->r_adj[5], (creature.get_race_info()->r_exp - 100));
        c_put_str(TERM_L_BLUE, stats, 5, 40);

        put_str("HD ", 6, 40);
        const auto hd = format("%2d", creature.get_race_info()->r_mhp);
        c_put_str(TERM_L_BLUE, hd, 6, 43);

        put_str(_("隠密", "Stealth"), 6, 47);
        const auto stealth = format("%+2d", creature.get_race_info()->r_stl);
        c_put_str(TERM_L_BLUE, stealth, 6, _(52, 55));

        put_str(_("赤外線視力", "Infra"), 6, _(56, 59));
        const auto infra = format(_("%%2dft", "%%2dft"), 10 * creature.get_race_info()->infra);
        c_put_str(TERM_L_BLUE, infra, 6, _(67, 65));
    }

    c_put_str(TERM_YELLOW, result, 12 + (cs / 5), 1 + 16 * (cs % 5));
    *os = cs;
    return result;
}

static void interpret_race_select_key_move(char c, int *cs, int count)
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
        if (*cs < count) {
            (*cs)++;
        }
    }

    if (c == '2') {
        if ((*cs + 5) <= count) {
            *cs += 5;
        }
    }
}

//!< 生 (raw) 種族インデックスを表示インデックスに変換 (見つからなければ 0)
static int raw_to_display_index(int raw, const std::vector<int> &playables)
{
    for (int i = 0; i < static_cast<int>(playables.size()); i++) {
        if (playables[i] == raw) {
            return i;
        }
    }
    return 0;
}

static bool select_race(CreatureEntity &creature, char *sym, int *k, const std::vector<int> &playables)
{
    const auto count = static_cast<int>(playables.size());
    auto cs = raw_to_display_index(enum2i(creature.prace), playables);
    int os = count;
    std::string cur = birth_race_label(os, sym, playables);
    while (true) {
        cur = display_race_stat(creature, cs, &os, cur, sym, playables);
        if (*k >= 0) {
            break;
        }

        const auto buf = format(_("種族を選んで下さい (%c-%c) ('='初期オプション設定): ", "Choose a race (%c-%c) ('=' for options): "), sym[0], sym[count - 1]);
        put_str(buf, 10, 10);
        char c = inkey();
        if (c == 'Q') {
            birth_quit();
        }

        if (c == 'S') {
            return false;
        }

        if (c == ' ' || c == '\r' || c == '\n') {
            if (cs == count) {
                cs = randint0(count);
                continue;
            } else {
                *k = playables[cs];
                break;
            }
        }

        if (c == '*') {
            cs = randint0(count);
            continue;
        }

        interpret_race_select_key_move(c, &cs, count);
        int disp = (islower(c) ? A2I(c) : -1);
        if ((disp >= 0) && (disp < count)) {
            cs = disp;
            continue;
        }

        disp = (isupper(c) ? (26 + c - 'A') : -1);
        if ((disp >= 26) && (disp < count)) {
            cs = disp;
            continue;
        }

        birth_help_option(creature, c, BirthKind::RACE);
    }

    return true;
}

/*!
 * @brief プレイヤーの種族選択を行う / Player race
 * @param creature クリーチャーへの参照
 */
bool get_player_race(CreatureEntity &creature)
{
    clear_from(10);
    put_str(
        _("注意：《種族》によってキャラクターの先天的な資質やボーナスが変化します。", "Note: Your 'race' determines various intrinsic factors and bonuses."),
        23, 5);

    const auto playables = get_playable_race_indices();
    char sym[MAX_RACES] = {};
    enumerate_race_list(creature, sym, playables);
    int k = -1;
    if (!select_race(creature, sym, &k, playables)) {
        return false;
    }

    creature.prace = i2enum<PlayerRaceType>(k);
    creature.race = &race_info[k];
    c_put_str(TERM_L_BLUE, creature.get_race_info()->title, 4, 15);
    return true;
}
