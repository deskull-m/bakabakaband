#include "birth/birth-wizard.h"
#include "avatar/avatar.h"
#include "birth/auto-roller.h"
#include "birth/birth-body-spec.h"
#include "birth/birth-explanations-table.h"
#include "birth/birth-select-class.h"
#include "birth/birth-select-patron.h"
#include "birth/birth-select-personality.h"
#include "birth/birth-select-race.h"
#include "birth/birth-select-realm.h"
#include "birth/birth-stat.h"
#include "birth/birth-util.h"
#include "birth/game-play-initializer.h"
#include "birth/history-editor.h"
#include "birth/history-generator.h"
#include "birth/quick-start.h"
#include "cmd-io/cmd-gameoption.h"
#include "cmd-io/cmd-help.h"
#include "core/asking-player.h"
#include "game-option/birth-options.h"
#include "game-option/game-option-page.h"
#include "io/input-key-acceptor.h"
#include "locale/japanese.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player-info/class-types.h"
#include "player-info/race-info.h"
#include "player/patron.h"
#include "player/player-personality.h"
#include "player/player-sex.h"
#include "player/player-status-table.h"
#include "player/player-status.h"
#include "player/process-name.h"
#include "system/creature-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "util/enum-converter.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-birth.h" // 暫定。後で消す予定。
#include "view/display-player-misc-info.h"
#include "view/display-player.h" // 暫定。後で消す.
#include "view/display-symbol.h"
#include "view/display-util.h"
#include "world/world.h"
#include <sstream>

static void display_initial_birth_message(CreatureEntity &creature)
{
    term_clear();
    display_player_name(creature, true);
    put_str(_("性別        :", "Sex         :"), 3, 1);
    put_str(_("種族        :", "Race        :"), 4, 1);
    put_str(_("職業        :", "Class       :"), 5, 1);
    put_str(_("キャラクターを作成します。('S'やり直す, 'Q'終了, '?'ヘルプ)", "Make your character. ('S' Restart, 'Q' Quit, '?' Help)"), 8, 10);
    put_str(_("注意：《性別》の違いはゲーム上ほとんど影響を及ぼしません。", "Note: Your 'sex' does not have any significant gameplay effects."), 23, 5);
}

/*!
 * @prief 性別選択画面でヘルプを表示させる
 * @param creature クリーチャーへの参照
 * @param c 入力したコマンド
 * @details 他の関数名と被りそうだったので少し眺め
 */
static void display_help_on_sex_select(CreatureEntity &creature, char c)
{
    if (c == '?') {
        do_cmd_help(creature);
    } else if (c == '=') {
        screen_save();
        do_cmd_options_aux(creature, GameOptionPage::BIRTH, _("初期オプション((*)はスコアに影響)", "Birth Options ((*)) affect score"));
        screen_load();
    } else if (c != '4' && c != '6') {
        bell();
    }
}

/*!
 * @brief 表示されている性別のラベルを取得する
 * @param cs 性別の指標
 * @return std::string 表示するラベルを保持
 */
static std::string birth_sex_label(int cs)
{
    const char p2 = ')';
    std::stringstream ss;

    if (cs < 0 || cs >= MAX_SEXES) {
        ss << '*' << p2 << _("ランダム", "Random");
    } else {
        ss << I2A(cs) << p2 << sex_info[cs].title;
    }
    return ss.str();
}

/*!
 * @brief プレイヤーの性別選択を行う / Player sex
 * @param creature クリーチャーへの参照
 * @return やり直すならFALSE、それ以外はTRUE
 */
static bool get_player_sex(CreatureEntity &creature)
{
    int k = -1;
    int cs = 0;
    int os = MAX_SEXES;
    auto cur = birth_sex_label(os);
    while (true) {
        if (cs != os) {
            put_str(cur, 12 + (os / 5), 2 + 15 * (os % 5));
            cur = birth_sex_label(cs);
            c_put_str(TERM_YELLOW, cur, 12 + (cs / 5), 2 + 15 * (cs % 5));
            os = cs;
        }

        if (k >= 0) {
            break;
        }

        const auto buf = format(_("性別を選んで下さい (%c-%c) ('='初期オプション設定): ", "Choose a sex (%c-%c) ('=' for options): "), I2A(0), I2A(1));
        put_str(buf, 10, 10);
        char c = inkey();
        if (c == 'Q') {
            birth_quit();
        }

        if (c == 'S') {
            return false;
        }

        if (c == ' ' || c == '\r' || c == '\n') {
            k = cs == MAX_SEXES ? randint0(MAX_SEXES) : cs;
            break;
        }

        if (c == '*') {
            k = randint0(MAX_SEXES);
            break;
        }

        if (c == '4') {
            if (cs > 0) {
                cs--;
            }
        }

        if (c == '6') {
            if (cs < MAX_SEXES) {
                cs++;
            }
        }

        k = (islower(c) ? A2I(c) : -1);
        if ((k >= 0) && (k < MAX_SEXES)) {
            cs = k;
            continue;
        } else {
            k = -1;
        }

        display_help_on_sex_select(creature, c);
    }

    creature.psex = i2enum<player_sex>(k);
    sp_ptr = &sex_info[creature.psex];
    c_put_str(TERM_L_BLUE, sp_ptr->title, 3, 15);
    return true;
}

static bool let_player_select_race(CreatureEntity &creature)
{
    clear_from(10);
    creature.prace = PlayerRaceType::HUMAN;
    while (true) {
        if (!get_player_race(creature)) {
            return false;
        }

        clear_from(10);
        display_wrap_around(race_explanations[enum2i(creature.prace)], 74, 12, 3);
        if (input_check_strict(creature, _("よろしいですか？", "Are you sure? "), UserCheck::DEFAULT_Y)) {
            break;
        }

        clear_from(10);
        c_put_str(TERM_WHITE, "              ", 4, 15);
    }

    return true;
}

static bool let_player_select_class(CreatureEntity &creature)
{
    clear_from(10);
    creature.pclass = PlayerClassType::WARRIOR;
    while (true) {
        if (!get_player_class(creature)) {
            return false;
        }

        clear_from(10);
        display_wrap_around(class_explanations[enum2i(creature.pclass)], 74, 12, 3);

        if (input_check_strict(creature, _("よろしいですか？", "Are you sure? "), UserCheck::DEFAULT_Y)) {
            break;
        }

        c_put_str(TERM_WHITE, "              ", 5, 15);
    }

    return true;
}

static bool let_player_select_personality(CreatureEntity &creature)
{
    creature.ppersonality = PERSONALITY_ORDINARY;
    while (true) {
        if (!get_player_personality(creature)) {
            return false;
        }

        clear_from(10);
        display_wrap_around(personality_explanations[creature.ppersonality], 74, 12, 3);

        if (input_check_strict(creature, _("よろしいですか？", "Are you sure? "), UserCheck::DEFAULT_Y)) {
            break;
        }

        display_player_name(creature, true);
    }

    return true;
}

static bool let_player_select_patron(CreatureEntity &creature)
{
    // 混沌の戦士以外はパトロンを選択しない
    if (creature.pclass != PlayerClassType::CHAOS_WARRIOR) {
        creature.patron = randnum0<short>(MAX_PATRON);
        return true;
    }

    creature.patron = 0; // 初期値
    while (true) {
        if (!get_player_patron(creature)) {
            return false;
        }

        clear_from(10);
        put_str(_("パトロンが選択されました。", "Patron selected."), 12, 3);

        if (input_check_strict(creature, _("よろしいですか？", "Are you sure? "), UserCheck::DEFAULT_Y)) {
            break;
        }

        display_player_name(creature, true);
    }

    return true;
}

static bool let_player_build_character(CreatureEntity &creature)
{
    if (!get_player_sex(creature)) {
        return false;
    }

    if (!let_player_select_race(creature)) {
        return false;
    }

    if (!let_player_select_class(creature)) {
        return false;
    }

    if (!get_player_realms(creature)) {
        return false;
    }

    if (!let_player_select_personality(creature)) {
        return false;
    }

    if (!let_player_select_patron(creature)) {
        return false;
    }

    CreatureClass(creature).init_specific_data();

    return true;
}

static void display_initial_options(CreatureEntity &creature)
{
    const auto expfact_mod = static_cast<int>(get_expfact(creature)) - 100;
    int16_t adj[A_MAX]{};
    for (int i = 0; i < A_MAX; i++) {
        adj[i] = creature.race->r_adj[i] + (*creature.pclass_ref).c_adj[i] + (*creature.personality).a_adj[i];
    }

    put_str("                                   ", 3, 40);
    put_str(_("修正の合計値", "Your total modification"), 3, 40);
    put_str(_("腕力 知能 賢さ 器用 耐久 魅力 経験 ", "Str  Int  Wis  Dex  Con  Chr   EXP "), 4, 40);
    const auto stats = format("%+3d  %+3d  %+3d  %+3d  %+3d  %+3d %+4d%% ", adj[0], adj[1], adj[2], adj[3], adj[4], adj[5], expfact_mod);
    c_put_str(TERM_L_BLUE, stats, 5, 40);

    put_str("HD ", 6, 40);
    const auto hd = format("%2d", creature.race->r_mhp + (*creature.pclass_ref).c_mhp + (*creature.personality).a_mhp);
    c_put_str(TERM_L_BLUE, hd, 6, 43);

    put_str(_("隠密", "Stealth"), 6, 47);
    std::string stealth;
    if (CreatureClass(creature).equals(PlayerClassType::BERSERKER)) {
        stealth = "xx";
    } else {
        stealth = format("%+2d", creature.race->r_stl + (*creature.pclass_ref).c_stl + (*creature.personality).a_stl);
    }
    c_put_str(TERM_L_BLUE, stealth, 6, _(52, 55));

    put_str(_("赤外線視力", "Infra"), 6, _(56, 59));
    const auto infra = format(_("%%2dft", "%%2dft"), 10 * creature.race->infra);
    c_put_str(TERM_L_BLUE, infra, 6, _(67, 65));

    clear_from(10);
    screen_save();
    do_cmd_options_aux(creature, GameOptionPage::BIRTH, _("初期オプション((*)はスコアに影響)", "Birth Options ((*)) affect score"));
    screen_load();
}

static void display_auto_roller_success_rate(CreatureEntity &creature, const int col)
{
    if (!autoroller) {
        return;
    }

    put_str(_("最小値", " Limit"), 2, col + 13);
    put_str(_("現在値", "  Roll"), 2, col + 24);

    std::string prob;

    if (autoroll_chance >= 1) {
        prob = format(_("確率 :  1/%8d00", "Prob :  1/%8d00"), autoroll_chance);
    } else if (autoroll_chance == -999) {
        prob = _("確率 :     不可能", "Prob :     Impossible");
    } else {
        prob = _("確率 :     1/10000以上", "Prob :     >1/10000");
    }
    put_str(prob, 11, col + 10);

    put_str(_("注意 : 体格等のオートローラを併用時は、上記確率より困難です。", "Note : Prob may be lower when you use the 'autochara' option."), 22, 5);

    for (int i = 0; i < A_MAX; i++) {
        put_str(stat_names[i], 3 + i, col + 8);
        int j = creature.race->r_adj[i] + (*creature.pclass_ref).c_adj[i] + (*creature.personality).a_adj[i];
        int m = adjust_stat(stat_limit[i], j);
        c_put_str(TERM_L_BLUE, cnv_stat(m), 3 + i, col + 13);
    }
}

static void auto_roller_count(void)
{
    if (auto_round < 1000000000L) {
        return;
    }

    auto_round = 1;
    if (!autoroller) {
        return;
    }

    auto_upper_round++;
}

static bool decide_initial_stat(CreatureEntity &creature)
{
    if (!autoroller) {
        return true;
    }

    bool accept = true;
    for (int i = 0; i < A_MAX; i++) {
        if (creature.stat_max[i] < stat_limit[i]) {
            accept = false;
            break;
        }
    }

    return accept;
}

static bool decide_body_spec(CreatureEntity &creature, chara_limit_type chara_limit, bool *accept)
{
    if (!*accept) {
        return false;
    }

    get_ahw(creature);
    get_history(creature);

    if (autochara) {
        if ((creature.age < chara_limit.agemin) || (creature.age > chara_limit.agemax)) {
            *accept = false;
        }
        const auto ht = _(inch_to_cm(creature.ht), creature.ht);
        if ((ht < chara_limit.htmin) || (ht > chara_limit.htmax)) {
            *accept = false;
        }
        const auto wt = _(lb_to_kg(creature.wt), creature.wt);
        if ((wt < chara_limit.wtmin) || (wt > chara_limit.wtmax)) {
            *accept = false;
            if ((creature.prestige < chara_limit.scmin) || (creature.prestige > chara_limit.scmax)) {
                *accept = false;
            }
        }
    }
    return *accept;
}

static bool display_auto_roller_count(CreatureEntity &creature, const int col)
{
    /*!
     * @details ここで指定された回数だけロールする度にその時の結果を画面に表示する
     * @todo この定数を定義した時代に比べて、CPUパワーが相当に上がっている.
     * 表示部で足を引っ張っているので、タイマで数えて0.1秒/表示 くらいで良いと思われる.
     */
    constexpr auto roll_results_per_display = 54321;
    if ((auto_round % roll_results_per_display) != 0) {
        return false;
    }

    birth_put_stats(creature);
    if (auto_upper_round) {
        put_str(format("%ld%09ld", auto_upper_round, auto_round), 10, col + 20);
    } else {
        put_str(format("%10ld", auto_round), 10, col + 20);
    }
    term_fresh();
    inkey_scan = true;
    if (inkey()) {
        get_ahw(creature);
        get_history(creature);
        return true;
    }

    return false;
}

static void exe_auto_roller(CreatureEntity &creature, chara_limit_type chara_limit, const int col)
{
    while (autoroller || autochara) {
        get_stats(creature);
        auto_round++;
        auto_roller_count();
        bool accept = decide_initial_stat(creature);
        if (decide_body_spec(creature, chara_limit, &accept)) {
            return;
        }

        if (display_auto_roller_count(creature, col)) {
            return;
        }
    }
}

static bool display_auto_roller_result(CreatureEntity &creature, bool prev, char *c)
{
    BIT_FLAGS mode = 0;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::HP,
    };
    while (true) {
        rfu.set_flags(flags);
        update_creature(creature);
        creature.hp = creature.maxhp;
        creature.csp = creature.msp;
        (void)display_player(creature, mode);
        term_gotoxy(2, 23);
        const char b1 = '[';
        term_addch({ TERM_WHITE, b1 });
        term_addstr(-1, TERM_WHITE, _("'r' 次の数値", "'r'eroll"));
        if (prev) {
            term_addstr(-1, TERM_WHITE, _(", 'p' 前の数値", ", 'p'revious"));
        }

        if (mode) {
            term_addstr(-1, TERM_WHITE, _(", 'h' その他の情報", ", 'h' Misc."));
        } else {
            term_addstr(-1, TERM_WHITE, _(", 'h' 生い立ちを表示", ", 'h'istory"));
        }

        term_addstr(-1, TERM_WHITE, _(", Enter この数値に決定", ", or Enter to accept"));
        const char b2 = ']';
        term_addch({ TERM_WHITE, b2 });
        *c = inkey();
        if (*c == 'Q') {
            birth_quit();
        }

        if (*c == 'S') {
            return false;
        }

        if (*c == '\r' || *c == '\n' || *c == ESCAPE) {
            break;
        }

        if ((*c == ' ') || (*c == 'r')) {
            break;
        }

        if (prev && (*c == 'p')) {
            load_prev_data(creature, true);
            continue;
        }

        if ((*c == 'H') || (*c == 'h')) {
            mode = ((mode != 0) ? 0 : 1);
            continue;
        }

        birth_help_option(creature, *c, BirthKind::AUTO_ROLLER);
        bell();
    }

    return true;
}

/*
 * @brief オートロールを回して結果を表示し、その数値に決めるかさらに回すか確認する。
 * @param creature クリーチャーへの参照
 * @param chara_limit 社会的地位の要求水準
 * @details 2つめの結果以降は、'p'キーで1つ前のロール結果に戻せる。
 */
static bool display_auto_roller(CreatureEntity &creature, chara_limit_type chara_limit)
{
    bool prev = false;

    while (true) {
        int col = 22;
        if (autoroller || autochara) {
            term_clear();
            put_str(_("回数 :", "Round:"), 10, col + 10);
            put_str(_("(ESCで停止)", "(Hit ESC to stop)"), 13, col + 13);
        } else {
            get_stats(creature);
            get_ahw(creature);
            get_history(creature);
        }

        display_auto_roller_success_rate(creature, col);
        exe_auto_roller(creature, chara_limit, col);
        if (autoroller || autochara) {
            sound(SoundKind::LEVEL);
        }

        flush();

        get_extra(creature, true);
        get_money(creature);

        char c;
        if (!display_auto_roller_result(creature, prev, &c)) {
            return false;
        }

        if (c == '\r' || c == '\n' || c == ESCAPE) {
            break;
        }

        save_prev_data(creature, &previous_char);
        previous_char.quick_ok = false;
        prev = true;
    }

    return true;
}

/*!
 * @brief 名前と生い立ちを設定する
 * @param creature クリーチャーへの参照
 * @details ついでにステータス限界もここで決めている
 */
static void set_name_history(CreatureEntity &creature)
{
    clear_from(23);
    get_name(creature);
    process_player_name(creature, AngbandWorld::get_instance().creating_savefile);
    edit_history(creature);
    get_max_stats(creature);
    initialize_virtues(creature);
    prt(_("[ 'Q' 中断, 'S' 初めから, Enter ゲーム開始 ]", "['Q'uit, 'S'tart over, or Enter to continue]"), 23, _(14, 10));
}

/*!
 * @brief プレイヤーキャラ作成ウィザード
 * @details
 * The delay may be reduced, but is recommended to keep players
 * from continuously rolling up characters, which can be VERY
 * expensive CPU wise.  And it cuts down on player stupidity.
 */
bool player_birth_wizard(CreatureEntity &creature)
{
    display_initial_birth_message(creature);
    for (int n = 0; n < MAX_SEXES; n++) {
        put_str(birth_sex_label(n), 12 + (n / 5), 2 + 15 * (n % 5));
    }

    if (!let_player_build_character(creature)) {
        return false;
    }

    display_initial_options(creature);
    if (autoroller || autochara) {
        auto_round = 0L;
        auto_upper_round = 0L;
        autoroll_chance = 0L;
    }

    if (autoroller) {
        if (!get_stat_limits(creature)) {
            return false;
        }
    }

    chara_limit_type chara_limit;
    initialize_chara_limit(&chara_limit);
    if (autochara) {
        if (!get_chara_limits(creature, &chara_limit)) {
            return false;
        }
    }

    clear_from(10);
    init_turn(creature);
    if (!display_auto_roller(creature, chara_limit)) {
        return false;
    }

    set_name_history(creature);
    char c = inkey();
    if (c == 'Q') {
        birth_quit();
    }

    if (c == 'S') {
        return false;
    }

    init_dungeon_quests(creature);
    save_prev_data(creature, &previous_char);
    previous_char.quick_ok = true;
    return true;
}
