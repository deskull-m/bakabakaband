#include "window/main-window-left-frame.h"
#include "floor/dungeon-feeling.h"
#include "game-option/special-options.h"
#include "game-option/text-display-options.h"
#include "monster/monster-timed-effects.h"
#include "player-base/player-race.h"
#include "player-info/class-info.h"
#include "player-info/mimic-info-table.h"
#include "player/player-status-table.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "timed-effect/timed-effects.h"
#include "tracking/health-bar-tracker.h"
#include "util/string-processor.h"
#include "window/main-window-row-column.h"
#include "window/main-window-stat-poster.h"
#include "window/main-window-util.h"
#include "world/world.h"
#include <fmt/format.h>

/*!
 * @brief ターゲットしているモンスターの情報部に表示する状態異常と文字色の対応を保持する構造体
 */
struct condition_layout_info {
    std::string label;
    TERM_COLOR color;
};

/*!
 * @brief プレイヤーの称号を表示する / Prints "title", including "wizard" or "winner" as needed.
 */
void print_title(CreatureEntity &creature)
{
    std::string p;
    const auto &world = AngbandWorld::get_instance();
    if (!creature.is_player()) {
        p = _("なし", "None");
    } else if (world.wizard) {
        p = _("［ウィザード］", "[=-WIZARD-=]");
    } else if (world.total_winner) {
        if (world.is_player_true_winner()) {
            p = _("*真・勝利者*", "*TRUEWINNER*");
        } else {
            p = _("***勝利者***", "***WINNER***");
        }
    } else {
        p = player_titles.at(creature.pclass).at((creature.level - 1) / 5);
    }

    print_field(p, ROW_TITLE, COL_TITLE);
}

/*!
 * @brief プレイヤーのレベルを表示する / Prints level
 */
void print_level(CreatureEntity &creature)
{
    const auto tmp = format("%5d", creature.level);
    if (creature.level >= creature.max_plv) {
        put_str(_("レベル ", "LEVEL "), ROW_LEVEL, 0);
        c_put_str(TERM_L_GREEN, tmp, ROW_LEVEL, COL_LEVEL + 7);
    } else {
        put_str(_("xレベル", "Level "), ROW_LEVEL, 0);
        c_put_str(TERM_YELLOW, tmp, ROW_LEVEL, COL_LEVEL + 7);
    }
}

/*!
 * @brief プレイヤーの経験値を表示する / Display the experience
 */
void print_exp(CreatureEntity &creature)
{
    std::string out_val;

    CreatureRace pr(&creature);
    if (!creature.is_player()) {
        // モンスターは player_exp 表に該当エントリを持たないため、
        // 現在経験値のみ表示する。
        out_val = format("%8d", creature.exp);
    } else if ((!exp_need) || pr.equals(PlayerRaceType::ANDROID)) {
        out_val = format("%8d", creature.exp);
    } else {
        if (creature.level >= PY_MAX_LEVEL) {
            (void)sprintf(out_val.data(), "********");
        } else {
            out_val = format("%8d", player_exp[creature.level - 1] * creature.expfact / 100 - creature.exp);
        }
    }

    if (creature.exp >= creature.max_exp) {
        if (pr.equals(PlayerRaceType::ANDROID)) {
            put_str(_("強化 ", "Cst "), ROW_EXP, 0);
        } else {
            put_str(_("経験 ", "EXP "), ROW_EXP, 0);
        }
        c_put_str(TERM_L_GREEN, out_val, ROW_EXP, COL_EXP + 4);
    } else {
        put_str(_("x経験", "Exp "), ROW_EXP, 0);
        c_put_str(TERM_YELLOW, out_val, ROW_EXP, COL_EXP + 4);
    }
}

/*!
 * @brief プレイヤーのACを表示する / Prints current AC
 */
void print_ac(CreatureEntity &creature)
{
    char tmp[32];

#ifdef JP
    /* AC の表示方式を変更している */
    put_str(" ＡＣ(     )", ROW_AC, COL_AC);
    sprintf(tmp, "%5d", creature.dis_ac + creature.dis_to_a);
    c_put_str(TERM_L_GREEN, tmp, ROW_AC, COL_AC + 6);
#else
    put_str("Cur AC ", ROW_AC, COL_AC);
    sprintf(tmp, "%5d", creature.dis_ac + creature.dis_to_a);
    c_put_str(TERM_L_GREEN, tmp, ROW_AC, COL_AC + 7);
#endif
}

/*!
 * @brief プレイヤーのHPを表示する / Prints Cur/Max hit points
 * @param creature クリーチャーへの参照
 */
void print_hp(CreatureEntity &creature)
{
    char tmp[32];
    put_str("HP", ROW_CURHP, COL_CURHP);
    sprintf(tmp, "%4ld", (long int)creature.hp);
    TERM_COLOR color;
    if (creature.hp >= creature.maxhp) {
        color = TERM_L_GREEN;
    } else if (creature.hp > (creature.maxhp * hitpoint_warn) / 10) {
        color = TERM_YELLOW;
    } else {
        color = TERM_RED;
    }

    c_put_str(color, format("%4d", creature.hp), ROW_CURHP, COL_CURHP + 3);
    put_str("/", ROW_CURHP, COL_CURHP + 7);
    sprintf(tmp, "%4ld", (long int)creature.maxhp);
    color = TERM_L_GREEN;
    c_put_str(color, format("%4d", creature.maxhp), ROW_CURHP, COL_CURHP + 8);
}

/*!
 * @brief MPを表示する / Prints max/cur spell points
 * @param creature クリーチャーへの参照
 * @details プレイヤー職業や msp の値に関わらず常に表示する。
 * モンスターは生成時に msp/csp とも 0 で初期化されるため 0/0 と表示される。
 */
void print_sp(CreatureEntity &creature)
{
    char tmp[32];
    byte color;

    put_str(_("MP", "SP"), ROW_CURSP, COL_CURSP);
    sprintf(tmp, "%4ld", (long int)creature.csp);
    if (creature.msp <= 0) {
        color = TERM_SLATE;
    } else if (creature.csp >= creature.msp) {
        color = TERM_L_GREEN;
    } else if (creature.csp > (creature.msp * mana_warn) / 10) {
        color = TERM_YELLOW;
    } else {
        color = TERM_RED;
    }

    c_put_str(color, format("%4d", creature.csp), ROW_CURSP, COL_CURSP + 3);
    put_str("/", ROW_CURSP, COL_CURSP + 7);
    sprintf(tmp, "%4ld", (long int)creature.msp);
    color = (creature.msp <= 0) ? TERM_SLATE : TERM_L_GREEN;
    c_put_str(color, format("%4d", creature.msp), ROW_CURSP, COL_CURSP + 8);
}

/*!
 * @brief プレイヤーの所持金を表示する / Prints current gold
 * @param creature クリーチャーへの参照
 */
void print_gold(CreatureEntity &creature)
{
    put_str(_("＄ ", "AU "), ROW_GOLD, COL_GOLD);
    c_put_str(TERM_L_GREEN, format("%9d", creature.au), ROW_GOLD, COL_GOLD + 3);
}

/*!
 * @brief 現在のフロアの深さを表示する / Prints depth in stat area
 * @param creature クリーチャーへの参照
 */
void print_depth(CreatureEntity &creature)
{
    std::string depths;
    TERM_COLOR attr = TERM_WHITE;
    const auto &[wid, hgt] = term_get_size();
    const auto col_depth = wid + COL_DEPTH;
    const auto row_depth = hgt + ROW_DEPTH;
    const auto &floor = *creature.get_floor();
    if (!floor.is_underground()) {
        c_prt(attr, format("%7s", _("地上", "Surf.")), row_depth, col_depth);
        return;
    }

    if (depth_in_feet) {
        depths = fmt::format(_("{} ft", "{} ft"), floor.dun_level * 50);
    } else {
        depths = fmt::format(_("{} 階", "Lev {}"), floor.dun_level);
    }

    switch (DungeonFeeling::get_instance().get_feeling()) {
    case 0:
        attr = TERM_SLATE;
        break; /* Unknown */
    case 1:
        attr = TERM_L_BLUE;
        break; /* Special */
    case 2:
        attr = TERM_VIOLET;
        break; /* Horrible visions */
    case 3:
        attr = TERM_RED;
        break; /* Very dangerous */
    case 4:
        attr = TERM_L_RED;
        break; /* Very bad feeling */
    case 5:
        attr = TERM_ORANGE;
        break; /* Bad feeling */
    case 6:
        attr = TERM_YELLOW;
        break; /* Nervous */
    case 7:
        attr = TERM_L_UMBER;
        break; /* Luck is turning */
    case 8:
        attr = TERM_L_WHITE;
        break; /* Don't like */
    case 9:
        attr = TERM_WHITE;
        break; /* Reasonably safe */
    case 10:
        attr = TERM_WHITE;
        break; /* Boring place */
    }

    c_prt(attr, depths.c_str(), row_depth, col_depth);
}

/*!
 * @brief プレイヤーのステータスを一括表示する（左側部分） / Display basic info (mostly left of map)
 * @param creature クリーチャーへの参照
 */
void print_frame_basic(CreatureEntity &creature)
{
    // モンスター時 race_info は nullptr なので種族名は monrace.name を使う。
    // どちらも取れない場合は「なし」表記。
    std::string_view title;
    if (creature.get_mimic_form() != MimicKindType::NONE) {
        title = mimic_info.at(creature.get_mimic_form()).title;
    } else if (const auto *race_info = creature.get_race_info(); race_info != nullptr) {
        title = race_info->title;
    } else if (creature.has_monster_profile()) {
        title = creature.get_monrace().name;
    } else {
        title = _("なし", "None");
    }
    print_field(title, ROW_RACE, COL_RACE);
    print_title(creature);
    print_level(creature);
    print_exp(creature);
    for (int i = 0; i < A_MAX; i++) {
        print_stat(creature, i);
    }

    print_ac(creature);
    print_hp(creature);
    print_sp(creature);
    print_gold(creature);
    print_depth(creature);
    print_health(creature, true);
    print_health(creature, false);
}

/*!
 * @brief wizardモード中の闘技場情報を表示する
 * @param creature クリーチャーへの参照
 */
static void print_health_monster_in_arena_for_wizard(CreatureEntity &creature)
{
    int row = ROW_INFO - 1;
    int col = COL_INFO + 2;

    const int max_num_of_monster_in_arena = 4;

    for (int i = 0; i < max_num_of_monster_in_arena; i++) {
        auto row_offset = i;
        auto monster_list_index = i + 1; // m_listの1-4に闘技場のモンスターデータが入っている

        term_putstr(col - 2, row + row_offset, 12, TERM_WHITE, "      /     ");

        auto &monster = creature.get_floor()->get_monster(static_cast<MONSTER_IDX>(monster_list_index));
        if (monster.is_valid()) {
            const auto &monrace = monster.get_monrace();
            const auto &symbol_config = monrace.symbol_config;
            term_putstr(col - 2, row + row_offset, 2, symbol_config.color, format("%c", symbol_config.character));
            term_putstr(col - 1, row + row_offset, 5, TERM_WHITE, format("%5d", monster.hp));
            term_putstr(col + 5, row + row_offset, 6, TERM_WHITE, format("%5d", monster.max_maxhp));
        }
    }
}

/*!
 * @brief 対象のモンスターからcondition_layout_infoのリストを生成して返す
 * @param monster 対象のモンスター
 * @return condition_layout_infoのリスト
 */
static std::vector<condition_layout_info> get_condition_layout_info(const CreatureEntity &monster)
{
    std::vector<condition_layout_info> result;

    if (monster.is_invulnerable()) {
        result.push_back({ effect_type_to_label.at(CreatureTimedEffect::INVULNERABILITY), TERM_WHITE });
    }
    if (monster.is_accelerated()) {
        result.push_back({ effect_type_to_label.at(CreatureTimedEffect::ACCELERATION), TERM_L_GREEN });
    }
    if (monster.is_decelerated()) {
        result.push_back({ effect_type_to_label.at(CreatureTimedEffect::DECELERATION), TERM_UMBER });
    }
    if (monster.is_fearful()) {
        result.push_back({ effect_type_to_label.at(CreatureTimedEffect::FEAR), TERM_SLATE });
    }
    if (monster.is_confused()) {
        result.push_back({ effect_type_to_label.at(CreatureTimedEffect::CONFUSION), TERM_L_UMBER });
    }
    if (monster.is_asleep()) {
        result.push_back({ effect_type_to_label.at(CreatureTimedEffect::SLEEP_OR_PARALYSIS), TERM_BLUE });
    }
    if (monster.is_stunned()) {
        result.push_back({ effect_type_to_label.at(CreatureTimedEffect::STUN), TERM_ORANGE });
    }

    return result;
}

/*!
 * @brief モンスターの体力ゲージを表示する
 * @param riding TRUEならば騎乗中のモンスターの体力、FALSEならターゲットモンスターの体力を表示する。表示位置は固定。
 * @details
 * <pre>
 * Redraw the "monster health bar"	-DRS-
 * Rather extensive modifications by	-BEN-
 *
 * The "monster health bar" provides visual feedback on the "health"
 * of the monster currently being "tracked".  There are several ways
 * to "track" a monster, including targetting it, attacking it, and
 * affecting it (and nobody else) with a ranged attack.
 *
 * Display the monster health bar (affectionately known as the
 * "health-o-meter").  Clear health bar if nothing is being tracked.
 * Auto-track current target monster when bored.  Note that the
 * health-bar stops tracking any monster that "disappears".
 * </pre>
 */
void print_health(CreatureEntity &creature, bool riding)
{
    tl::optional<short> monster_idx;
    int row, col;

    if (riding) {
        if (creature.get_riding() > 0) {
            monster_idx = creature.get_riding();
        }
        row = ROW_RIDING_INFO;
        col = COL_RIDING_INFO;
    } else {
        // ウィザードモードで闘技場観戦時の表示
        if (AngbandWorld::get_instance().wizard && AngbandSystem::get_instance().is_phase_out()) {
            print_health_monster_in_arena_for_wizard(creature);
            return;
        }

        auto &tracker = HealthBarTracker::get_instance();
        if (tracker.is_tracking()) {
            monster_idx = tracker.get_trackee();
        }
        row = ROW_INFO;
        col = COL_INFO;
    }

    const auto max_width = 12; // 表示幅
    const auto &[wid, hgt] = term_get_size();
    const auto extra_line_count = riding ? 0 : hgt - MAIN_TERM_MIN_ROWS;
    for (auto y = row; y < row + extra_line_count + 1; ++y) {
        term_erase(col, y, max_width);
    }

    if (!monster_idx.has_value()) {
        return;
    }

    const auto &monster = creature.get_floor()->get_monster(monster_idx.value());

    if ((!monster.is_visible_on_map()) || (creature.is_hallucinated()) || monster.is_dead()) {
        term_putstr(col, row, max_width, TERM_WHITE, "[----------]");
        return;
    }

    const auto &[hit_point_bar_color, len] = monster.get_hp_bar_data();
    term_putstr(col, row, max_width, TERM_WHITE, "[----------]");
    term_putstr(col + 1, row, len, hit_point_bar_color, "**********");

    // 騎乗中のモンスターの状態異常は表示しない
    if (riding) {
        return;
    }

    int col_offset = 0;
    int row_offset = 1;

    // 一時的状態異常
    // MAX_WIDTHを超えたら次の行に移動する
    for (const auto &info : get_condition_layout_info(monster)) {
        if (row_offset > extra_line_count) {
            break;
        }
        if (col_offset + info.label.length() > max_width) { // 改行が必要かどうかチェック
            col_offset = 0;
            row_offset++;
        }
        term_putstr(col + col_offset, row + row_offset, max_width, info.color, info.label);
        col_offset += info.label.length() + 1; // 文字数と空白の分だけoffsetを加算
    }
}
