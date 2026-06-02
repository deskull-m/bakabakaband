#include "birth/birth-select-class.h"
#include "birth/birth-util.h"
#include "io/input-key-acceptor.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "system/creature-entity.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "world/world.h"
#include <sstream>
#include <vector>

//!< 選択可能 (playable) な職業の生 (raw) インデックス一覧。
//!< メニューはこの一覧上の「表示インデックス」で操作し、生インデックスとは
//!< 都度この一覧経由で変換する。これにより playable 職業が enum 上で連続して
//!< いなくても (例: 末尾に NPC 専用職業を追加した場合) 正しく表示・選択でき、
//!< カーソルが非選択職業のマスへ迷い込むこともない。
static std::vector<int> get_playable_class_indices()
{
    std::vector<int> playable_classes;
    for (auto i = 0; i < PLAYER_CLASS_TYPE_MAX; i++) {
        if (class_info.at(i2enum<PlayerClassType>(i)).playable) {
            playable_classes.push_back(i);
        }
    }

    return playable_classes;
}

//!< 生 (raw) 職業インデックスを表示インデックスに変換 (見つからなければ 0)
static int raw_to_display_index(int raw, const std::vector<int> &playables)
{
    for (auto i = 0; i < static_cast<int>(playables.size()); i++) {
        if (playables[i] == raw) {
            return i;
        }
    }
    return 0;
}

static std::string birth_class_label(CreatureEntity &creature, int display_index, concptr sym, const std::vector<int> &playables)
{
    constexpr auto p2 = ')';
    std::stringstream ss;
    if (display_index < 0 || display_index >= static_cast<int>(playables.size())) {
        ss << '*' << p2 << _("ランダム", "Random");
        return ss.str();
    }

    const auto raw = playables[display_index];
    ss << sym[display_index] << p2;
    const auto pclass = i2enum<PlayerClassType>(raw);
    const auto title = class_info.at(pclass).title;
    if (!(creature.get_race_info()->choice & (1UL << raw))) {
        ss << '(' << title << ')';
    } else {
        ss << title;
    }

    return ss.str();
}

static void enumerate_class_list(CreatureEntity &creature, char *sym, const std::vector<int> &playables)
{
    for (auto display_index = 0; display_index < static_cast<int>(playables.size()); display_index++) {
        const auto raw = playables[display_index];
        cp_ptr = &class_info.at(i2enum<PlayerClassType>(raw));
        creature.pclass_ref = &class_info.at(i2enum<PlayerClassType>(raw));
        mp_ptr = &class_magics_info[raw];
        if (display_index < 26) {
            sym[display_index] = I2A(display_index);
        } else {
            sym[display_index] = ('A' + display_index - 26);
        }

        auto cs = i2enum<PlayerClassType>(raw);
        c_put_str(AngbandWorld::get_instance().get_birth_class_color(cs), birth_class_label(creature, display_index, sym, playables), 13 + (display_index / 4), 2 + 19 * (display_index % 4));
    }
}

static std::string display_class_stat(CreatureEntity &creature, int cs, int *os, const std::string &cur, concptr sym, const std::vector<int> &playables)
{
    if (cs == *os) {
        return cur;
    }

    const auto count = static_cast<int>(playables.size());
    auto pclass = i2enum<PlayerClassType>(*os < count ? playables[*os] : 0);
    c_put_str(AngbandWorld::get_instance().get_birth_class_color(pclass), cur, 13 + (*os / 4), 2 + 19 * (*os % 4));
    put_str("                                   ", 3, 40);
    auto result = birth_class_label(creature, cs, sym, playables);
    if (cs == count) {
        put_str("                                   ", 4, 40);
        put_str("                                   ", 5, 40);
        put_str("                                   ", 6, 40);
    } else {
        const auto raw = playables[cs];
        cp_ptr = &class_info.at(i2enum<PlayerClassType>(raw));
        creature.pclass_ref = &class_info.at(i2enum<PlayerClassType>(raw));
        mp_ptr = &class_magics_info[raw];

        const auto &class_ref = *creature.pclass_ref;
        c_put_str(TERM_L_BLUE, class_ref.title, 3, 40);
        put_str(_("の職業修正", ": Class modification"), 3, 40 + class_ref.title->length());
        put_str(_("腕力 知能 賢さ 器用 耐久 魅力 経験 ", "Str  Int  Wis  Dex  Con  Chr   EXP "), 4, 40);
        const auto stats = format("%+3d  %+3d  %+3d  %+3d  %+3d  %+3d %+4d%% ", class_ref.c_adj[0], class_ref.c_adj[1], class_ref.c_adj[2], class_ref.c_adj[3], class_ref.c_adj[4], class_ref.c_adj[5], class_ref.c_exp);
        c_put_str(TERM_L_BLUE, stats, 5, 40);

        put_str("HD", 6, 40);
        const auto hd = format("%+3d", class_ref.c_mhp);
        c_put_str(TERM_L_BLUE, hd, 6, 42);

        put_str(_("隠密", "Stealth"), 6, 47);
        std::string stealth;
        if (i2enum<PlayerClassType>(playables[cs]) == PlayerClassType::BERSERKER) {
            stealth = " xx";
        } else {
            stealth = format(" %+2d", class_ref.c_stl);
        }
        c_put_str(TERM_L_BLUE, stealth, 6, _(51, 54));
    }

    c_put_str(TERM_YELLOW, result, 13 + (cs / 4), 2 + 19 * (cs % 4));
    *os = cs;
    return result;
}

static bool interpret_class_select_key_move(char c, int *cs, int count)
{
    if (c == '8') {
        if (*cs >= 4) {
            *cs -= 4;
        }
        return true;
    }

    if (c == '4') {
        if (*cs > 0) {
            (*cs)--;
        }
        return true;
    }

    if (c == '6') {
        if (*cs < count) {
            (*cs)++;
        }
        return true;
    }

    if (c == '2') {
        if (*cs + 4 <= count) {
            *cs += 4;
        }
        return true;
    }

    return false;
}

static bool select_class(CreatureEntity &creature, concptr sym, int *k, const std::vector<int> &playables)
{
    const auto count = static_cast<int>(playables.size());
    auto cs = raw_to_display_index(enum2i(creature.pclass), playables);
    int os = count;
    auto cur = birth_class_label(creature, os, sym, playables);
    while (true) {
        cur = display_class_stat(creature, cs, &os, cur, sym, playables);
        if (*k >= 0) {
            break;
        }

        const auto buf = format(_("職業を選んで下さい (%c-%c) ('='初期オプション設定, 灰色:勝利済): ", "Choose a class (%c-%c) ('=' for options, Gray is winner): "), sym[0], sym[count - 1]);

        put_str(buf, 10, 6);
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

        if (interpret_class_select_key_move(c, &cs, count)) {
            continue;
        }

        if (c == '*') {
            cs = randint0(count);
            continue;
        }

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

        birth_help_option(creature, c, BirthKind::CLASS);
    }

    return true;
}

/*!
 * @brief プレイヤーの職業選択を行う / Player class
 */
bool get_player_class(CreatureEntity &creature)
{
    clear_from(10);
    put_str(
        _("注意：《職業》によってキャラクターの先天的な能力やボーナスが変化します。", "Note: Your 'class' determines various intrinsic abilities and bonuses."),
        23, 5);
    put_str(_("()で囲まれた選択肢はこの種族には似合わない職業です。", "Any entries in parentheses should only be used by advanced players."), 11, 5);
    put_str("                                   ", 6, 40);

    const auto playables = get_playable_class_indices();
    char sym[PLAYER_CLASS_TYPE_MAX] = {};
    enumerate_class_list(creature, sym, playables);

    int k = -1;
    if (!select_class(creature, sym, &k, playables)) {
        return false;
    }

    creature.pclass = i2enum<PlayerClassType>(k);
    cp_ptr = &class_info.at(creature.pclass);
    creature.pclass_ref = &class_info.at(creature.pclass);
    mp_ptr = &class_magics_info[enum2i(creature.pclass)];
    c_put_str(TERM_L_BLUE, creature.get_class_info()->title, 5, 15);
    return true;
}
