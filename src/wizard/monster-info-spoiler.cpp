#include "wizard/monster-info-spoiler.h"
#include "io/files-util.h"
#include "monster-race/monster-race.h"
#include "system/angband-version.h"
#include "system/monster-race-info.h"
#include "term/term-color-types.h"
#include "util/angband-files.h"
#include "util/bit-flags-calculator.h"
#include "util/sort.h"
#include "util/string-processor.h"
#include "view/display-lore.h"
#include "view/display-messages.h"

/*!
 * @brief シンボル職の記述名を返す
 * @param monrace モンスター種族の構造体ポインタ
 * @return シンボル職の記述名
 * @todo MonsterRaceInfo のオブジェクトメソッドへ繰り込む
 */
static std::string attr_to_text(const MonsterRaceInfo &monrace)
{
    if (monrace.visual_flags.has(MonsterVisualType::CLEAR_COLOR)) {
        return _("透明な", "Clear");
    }

    if (monrace.visual_flags.has(MonsterVisualType::MULTI_COLOR)) {
        return _("万色の", "Multi");
    }

    if (monrace.visual_flags.has(MonsterVisualType::RANDOM_COLOR)) {
        return _("準ランダムな", "S.Rand");
    }

    switch (monrace.cc_def.color) {
    case TERM_DARK:
        return _("黒い", "Dark");
    case TERM_WHITE:
        return _("白い", "White");
    case TERM_SLATE:
        return _("青灰色の", "Slate");
    case TERM_ORANGE:
        return _("オレンジの", "Orange");
    case TERM_RED:
        return _("赤い", "Red");
    case TERM_GREEN:
        return _("緑の", "Green");
    case TERM_BLUE:
        return _("青い", "Blue");
    case TERM_UMBER:
        return _("琥珀色の", "Umber");
    case TERM_L_DARK:
        return _("灰色の", "L.Dark");
    case TERM_L_WHITE:
        return _("明るい青灰色の", "L.Slate");
    case TERM_VIOLET:
        return _("紫の", "Violet");
    case TERM_YELLOW:
        return _("黄色の", "Yellow");
    case TERM_L_RED:
        return _("明るい赤の", "L.Red");
    case TERM_L_GREEN:
        return _("明るい緑の", "L.Green");
    case TERM_L_BLUE:
        return _("明るい青の", "L.Blue");
    case TERM_L_UMBER:
        return _("明るい琥珀色の", "L.Umber");
    }

    return _("変な色の", "Icky");
}

SpoilerOutputResultType spoil_mon_desc(concptr fname, std::function<bool(const MonsterRaceInfo *)> filter_monster)
{
    PlayerType dummy;
    uint16_t why = 2;
    const auto path = path_build(ANGBAND_DIR_USER, fname);
    std::ofstream ofs(path);
    if (!ofs) {
        return SpoilerOutputResultType::FILE_OPEN_FAILED;
    }

    fprintf(spoiler_file, "Monster Spoilers for %s\n", get_version().data());
    fprintf(spoiler_file, "------------------------------------------\n\n");
    fprintf(spoiler_file, "%-45.45s%4s %4s %4s %7s %7s  %19.19s\n", "Name", "Lev", "Rar", "Spd", "Hp", "Ac", "Visual Info");
    fprintf(spoiler_file, "%-45.45s%4s %4s %4s %7s %7s  %4.19s\n",
        "---------------------------------------------"
        "----"
        "----------",
        "---", "---", "---", "-----", "-----", "-------------------");

    std::vector<MonsterRaceId> who;
    for (const auto &[monrace_id, monrace] : monraces_info) {
        if (MonsterRace(monrace_id).is_valid()) {
            who.push_back(monrace_id);
        }
    }

    ang_sort(&dummy, who.data(), &why, who.size(), ang_sort_comp_hook, ang_sort_swap_hook);
    for (auto r_idx : who) {
        const auto &monrace = monraces_info[r_idx];
        if (filter_monster && !filter_monster(&monrace)) {
            continue;
        }

        if (monrace.misc_flags.has(MonsterMiscType::KAGE)) {
            continue;
        }

        const auto name = str_separate(monrace.name, 40);
        std::string nam;
        if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
            nam = "[U] ";
        } else if (monrace.population_flags.has(MonsterPopulationType::NAZGUL)) {
            nam = "[N] ";
        } else {
            nam = _("    ", "The ");
        }
        nam.append(name.front());

        const auto lev = format("%d", monrace.level);
        const auto rar = format("%d", (int)monrace.rarity);
        const auto spd = format("%+d", monrace.speed - STANDARD_SPEED);
        const auto ac = format("%d", monrace.ac);
        std::string hp;
        if (monrace.misc_flags.has(MonsterMiscType::FORCE_MAXHP) || (monrace.hside == 1)) {
            hp = format("%d", monrace.hdice * monrace.hside);
        } else {
            hp = format("%dd%d", monrace.hdice, monrace.hside);
        }

        const auto symbol = format("%s '%c'", attr_to_text(monrace).data(), monrace.cc_def.character);
        ofs << format("%-45.45s%4s %4s %4s %7s %7s  %19.19s\n",
            nam.data(), lev.data(), rar.data(), spd.data(), hp.data(),
            ac.data(), symbol.data());

        for (auto i = 1U; i < name.size(); ++i) {
            fprintf(spoiler_file, "    %s\n", name[i].data());
        }
    }

    fprintf(spoiler_file, "\n");
    return ferror(spoiler_file) || angband_fclose(spoiler_file) ? SpoilerOutputResultType::FILE_CLOSE_FAILED
                                                                : SpoilerOutputResultType::SUCCESSFUL;
}

/*!
 * @brief 関数ポインタ用の出力関数 /
 * Hook function used in spoil_mon_info()
 * @param attr 未使用
 * @param str 文字列参照ポインタ
 */
static void roff_func(TERM_COLOR attr, std::string_view str)
{
    (void)attr;
    spoil_out(str.data());
}

/*!
 * @brief モンスター詳細情報をスポイラー出力するメインルーチン /
 * Create a spoiler file for monsters (-SHAWN-)
 * @param fname ファイル名
 */
SpoilerOutputResultType spoil_mon_info()
{
    PlayerType dummy;
    const auto path = path_build(ANGBAND_DIR_USER, "mon-info.txt");
    spoiler_file = angband_fopen(path, FileOpenMode::WRITE);
    if (!spoiler_file) {
        return SpoilerOutputResultType::FILE_OPEN_FAILED;
    }

    spoil_out(std::string("Monster Spoilers for ").append(get_version()).append("\n"));
    spoil_out("------------------------------------------\n\n");

    std::vector<MonsterRaceId> who;
    for (const auto &[monrace_id, monrace] : monraces_info) {
        if (MonsterRace(monrace_id).is_valid()) {
            who.push_back(monrace_id);
        }
    }

    uint16_t why = 2;
    ang_sort(&dummy, who.data(), &why, who.size(), ang_sort_comp_hook, ang_sort_swap_hook);
    for (auto r_idx : who) {
        const auto &monrace = monraces_info[r_idx];
        if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
            spoil_out("[U] ");
        } else if (monrace.population_flags.has(MonsterPopulationType::NAZGUL)) {
            spoil_out("[N] ");
        }

        spoil_out(format(_("%s/%s  (", "%s%s ("), monrace.name.data(), _(monrace.E_name.data(), ""))); /* ---)--- */
        spoil_out(attr_to_text(monrace));
        spoil_out(format(" '%c')\n", monrace.cc_def.character));
        spoil_out("=== ");
        spoil_out(format("Num:%d  ", enum2i(r_idx)));
        spoil_out(format("Lev:%d  ", (int)monrace.level));
        spoil_out(format("Rar:%d  ", monrace.rarity));
        spoil_out(format("Spd:%+d  ", monrace.speed - STANDARD_SPEED));
        if (monrace.misc_flags.has(MonsterMiscType::FORCE_MAXHP) || (monrace.hside == 1)) {
            spoil_out(format("Hp:%d  ", monrace.hdice * monrace.hside));
        } else {
            spoil_out(format("Hp:%dd%d  ", monrace.hdice, monrace.hside));
        }

        spoil_out(format("Ac:%d  ", monrace.ac));
        spoil_out(format("Exp:%ld\n", (long)(monrace.mexp)));
        output_monster_spoiler(r_idx, roff_func);
        spoil_out({}, true);
    }

    return ferror(spoiler_file) || angband_fclose(spoiler_file) ? SpoilerOutputResultType::FILE_CLOSE_FAILED
                                                                : SpoilerOutputResultType::SUCCESSFUL;
}
