﻿#include "window/main-window-left-frame.h"
#include "game-option/special-options.h"
#include "game-option/text-display-options.h"
#include "market/arena-info-table.h"
#include "monster-race/monster-race.h"
#include "monster/monster-status.h"
#include "player-base/player-race.h"
#include "player-info/class-info.h"
#include "player-info/mimic-info-table.h"
#include "player/player-status-table.h"
#include "system/floor-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "timed-effect/player-hallucination.h"
#include "timed-effect/timed-effects.h"
#include "util/string-processor.h"
#include "window/main-window-row-column.h"
#include "window/main-window-stat-poster.h"
#include "window/main-window-util.h"
#include "world/world.h"

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
void print_title(PlayerType *player_ptr)
{
    GAME_TEXT str[14];
    concptr p = "";
    if (w_ptr->wizard) {
        p = _("[ウィザード]", "[=-WIZARD-=]");
    } else if (w_ptr->total_winner) {
        if (player_ptr->is_true_winner()) {
            p = _("*真・勝利者*", "*TRUEWINNER*");
        } else {
            p = _("***勝利者***", "***WINNER***");
        }
    } else {
        angband_strcpy(str, player_titles[enum2i(player_ptr->pclass)][(player_ptr->lev - 1) / 5].data(), sizeof(str));
        p = str;
    }

    print_field(p, ROW_TITLE, COL_TITLE);
}

/*!
 * @brief プレイヤーのレベルを表示する / Prints level
 */
void print_level(PlayerType *player_ptr)
{
    char tmp[32];
    sprintf(tmp, "%5d", player_ptr->lev);
    if (player_ptr->lev >= player_ptr->max_plv) {
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
void print_exp(PlayerType *player_ptr)
{
    char out_val[32];

    PlayerRace pr(player_ptr);
    if ((!exp_need) || pr.equals(PlayerRaceType::ANDROID)) {
        (void)sprintf(out_val, "%8ld", (long)player_ptr->exp);
    } else {
        if (player_ptr->lev >= PY_MAX_LEVEL) {
            (void)sprintf(out_val, "********");
        } else {
            (void)sprintf(out_val, "%8ld", (long)(player_exp[player_ptr->lev - 1] * player_ptr->expfact / 100L) - player_ptr->exp);
        }
    }

    if (player_ptr->exp >= player_ptr->max_exp) {
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
void print_ac(PlayerType *player_ptr)
{
    char tmp[32];

#ifdef JP
    /* AC の表示方式を変更している */
    put_str(" ＡＣ(     )", ROW_AC, COL_AC);
    sprintf(tmp, "%5d", player_ptr->dis_ac + player_ptr->dis_to_a);
    c_put_str(TERM_L_GREEN, tmp, ROW_AC, COL_AC + 6);
#else
    put_str("Cur AC ", ROW_AC, COL_AC);
    sprintf(tmp, "%5d", player_ptr->dis_ac + player_ptr->dis_to_a);
    c_put_str(TERM_L_GREEN, tmp, ROW_AC, COL_AC + 7);
#endif
}

/*!
 * @brief プレイヤーのHPを表示する / Prints Cur/Max hit points
 */
void print_hp(PlayerType *player_ptr)
{
    char tmp[32];
    put_str("HP", ROW_CURHP, COL_CURHP);
    sprintf(tmp, "%4ld", (long int)player_ptr->chp);
    TERM_COLOR color;
    if (player_ptr->chp >= player_ptr->mhp) {
        color = TERM_L_GREEN;
    } else if (player_ptr->chp > (player_ptr->mhp * hitpoint_warn) / 10) {
        color = TERM_YELLOW;
    } else {
        color = TERM_RED;
    }

    c_put_str(color, tmp, ROW_CURHP, COL_CURHP + 3);
    put_str("/", ROW_CURHP, COL_CURHP + 7);
    sprintf(tmp, "%4ld", (long int)player_ptr->mhp);
    color = TERM_L_GREEN;
    c_put_str(color, tmp, ROW_CURHP, COL_CURHP + 8);
}

/*!
 * @brief プレイヤーのMPを表示する / Prints players max/cur spell points
 */
void print_sp(PlayerType *player_ptr)
{
    char tmp[32];
    byte color;
    if ((mp_ptr->spell_book == ItemKindType::NONE) && mp_ptr->spell_first == SPELL_FIRST_NO_SPELL) {
        return;
    }

    put_str(_("MP", "SP"), ROW_CURSP, COL_CURSP);
    sprintf(tmp, "%4ld", (long int)player_ptr->csp);
    if (player_ptr->csp >= player_ptr->msp) {
        color = TERM_L_GREEN;
    } else if (player_ptr->csp > (player_ptr->msp * mana_warn) / 10) {
        color = TERM_YELLOW;
    } else {
        color = TERM_RED;
    }

    c_put_str(color, tmp, ROW_CURSP, COL_CURSP + 3);
    put_str("/", ROW_CURSP, COL_CURSP + 7);
    sprintf(tmp, "%4ld", (long int)player_ptr->msp);
    color = TERM_L_GREEN;
    c_put_str(color, tmp, ROW_CURSP, COL_CURSP + 8);
}

/*!
 * @brief プレイヤーの所持金を表示する / Prints current gold
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void print_gold(PlayerType *player_ptr)
{
    char tmp[32];
    put_str(_("＄ ", "AU "), ROW_GOLD, COL_GOLD);
    sprintf(tmp, "%9ld", (long)player_ptr->au);
    c_put_str(TERM_L_GREEN, tmp, ROW_GOLD, COL_GOLD + 3);
}

/*!
 * @brief 現在のフロアの深さを表示する / Prints depth in stat area
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void print_depth(PlayerType *player_ptr)
{
    char depths[32];
    TERM_COLOR attr = TERM_WHITE;

    TERM_LEN wid, hgt;
    term_get_size(&wid, &hgt);
    TERM_LEN col_depth = wid + COL_DEPTH;
    TERM_LEN row_depth = hgt + ROW_DEPTH;

    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (!floor_ptr->dun_level) {
        strcpy(depths, _("地上", "Surf."));
        c_prt(attr, format("%7s", depths), row_depth, col_depth);
        return;
    }

    if (inside_quest(floor_ptr->quest_number) && !player_ptr->dungeon_idx) {
        strcpy(depths, _("地上", "Quest"));
        c_prt(attr, format("%7s", depths), row_depth, col_depth);
        return;
    }

    if (depth_in_feet) {
        (void)sprintf(depths, _("%d ft", "%d ft"), (int)floor_ptr->dun_level * 50);
    } else {
        (void)sprintf(depths, _("%d 階", "Lev %d"), (int)floor_ptr->dun_level);
    }

    switch (player_ptr->feeling) {
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

    c_prt(attr, format("%7s", depths), row_depth, col_depth);
}

/*!
 * @brief プレイヤーのステータスを一括表示する（左側部分） / Display basic info (mostly left of map)
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void print_frame_basic(PlayerType *player_ptr)
{
    if (player_ptr->mimic_form != MimicKindType::NONE) {
        print_field(mimic_info.at(player_ptr->mimic_form).title, ROW_RACE, COL_RACE);
    } else {
        char str[14];
        angband_strcpy(str, rp_ptr->title, sizeof(str));
        print_field(str, ROW_RACE, COL_RACE);
    }

    print_title(player_ptr);
    print_level(player_ptr);
    print_exp(player_ptr);
    for (int i = 0; i < A_MAX; i++) {
        print_stat(player_ptr, i);
    }

    print_ac(player_ptr);
    print_hp(player_ptr);
    print_sp(player_ptr);
    print_gold(player_ptr);
    print_depth(player_ptr);
    print_health(player_ptr, true);
    print_health(player_ptr, false);
}

/*!
 * @brief wizardモード中の闘技場情報を表示する
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void print_health_monster_in_arena_for_wizard(PlayerType *player_ptr)
{
    int row = ROW_INFO - 1;
    int col = COL_INFO + 2;

    const int max_num_of_monster_in_arena = 4;

    for (int i = 0; i < max_num_of_monster_in_arena; i++) {
        auto row_offset = i;
        auto monster_list_index = i + 1; // m_listの1-4に闘技場のモンスターデータが入っている

        term_putstr(col - 2, row + row_offset, 12, TERM_WHITE, "      /     ");

        auto &monster = player_ptr->current_floor_ptr->m_list[monster_list_index];
        if (MonsterRace(monster.r_idx).is_valid()) {
            term_putstr(col - 2, row + row_offset, 2, monraces_info[monster.r_idx].x_attr,
                format("%c", monraces_info[monster.r_idx].x_char));
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
static std::vector<condition_layout_info> get_condition_layout_info(const MonsterEntity &monster)
{
    std::vector<condition_layout_info> result;

    if (monster.is_invulnerable()) {
        result.push_back({ effect_type_to_label.at(MTIMED_INVULNER), TERM_WHITE });
    }
    if (monster.is_accelerated()) {
        result.push_back({ effect_type_to_label.at(MTIMED_FAST), TERM_L_GREEN });
    }
    if (monster.is_decelerated()) {
        result.push_back({ effect_type_to_label.at(MTIMED_SLOW), TERM_UMBER });
    }
    if (monster.is_fearful()) {
        result.push_back({ effect_type_to_label.at(MTIMED_MONFEAR), TERM_SLATE });
    }
    if (monster.is_confused()) {
        result.push_back({ effect_type_to_label.at(MTIMED_CONFUSED), TERM_L_UMBER });
    }
    if (monster.is_asleep()) {
        result.push_back({ effect_type_to_label.at(MTIMED_CSLEEP), TERM_BLUE });
    }
    if (monster.is_stunned()) {
        result.push_back({ effect_type_to_label.at(MTIMED_STUNNED), TERM_ORANGE });
    }

    return result;
}

/*!
 * @brief 対象のモンスターの状態（無敵、起きているか、HPの割合）に応じてHPバーの色を算出する
 * @param monster 対象のモンスター
 * @return HPバーの色
 */
static TERM_COLOR get_monster_hp_point_bar_color(const MonsterEntity &monster)
{
    auto pct = monster.maxhp > 0 ? 100 * monster.hp / monster.maxhp : 0;

    if (monster.is_invulnerable()) {
        return TERM_WHITE;
    }
    if (monster.is_asleep()) {
        return TERM_BLUE;
    }
    if (pct >= 100) {
        return TERM_L_GREEN;
    }
    if (pct >= 60) {
        return TERM_YELLOW;
    }
    if (pct >= 25) {
        return TERM_ORANGE;
    }
    if (pct >= 10) {
        return TERM_L_RED;
    }
    return TERM_RED;
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
void print_health(PlayerType *player_ptr, bool riding)
{
    std::optional<short> monster_idx;
    int row, col;

    if (riding) {
        if (player_ptr->riding > 0) {
            monster_idx = player_ptr->riding;
        }
        row = ROW_RIDING_INFO;
        col = COL_RIDING_INFO;
    } else {
        // ウィザードモードで闘技場観戦時の表示
        if (w_ptr->wizard && player_ptr->phase_out) {
            print_health_monster_in_arena_for_wizard(player_ptr);
            return;
        }
        if (player_ptr->health_who > 0) {
            monster_idx = player_ptr->health_who;
        }
        row = ROW_INFO;
        col = COL_INFO;
    }

    const int max_height_riding = 3; // 騎乗時に描画する高さの最大範囲
    const int max_height = 6; // 通常時に描画する高さの最大範囲
    const int max_width = 12; // 表示幅

    // 表示範囲を一旦全部消す
    int range = riding ? max_height_riding : max_height;
    for (int y = row; y < row + range; y++) {
        term_erase(col, y, max_width);
    }

    if (!monster_idx.has_value()) {
        return;
    }

    const int row_offset_name = 0; // 名前
    const int row_offset_health = 1; // HP
    const int row_offset_condition = 2; // 状態異常

    const auto &monster = player_ptr->current_floor_ptr->m_list[monster_idx.value()];

    if ((!monster.ml) || (player_ptr->effects()->hallucination()->is_hallucinated()) || monster.is_dead()) {
        term_putstr(col, row + row_offset_health, max_width, TERM_WHITE, "[----------]");
        return;
    }

    // HPの割合計算
    int pct2 = monster.maxhp > 0 ? 100L * monster.hp / monster.max_maxhp : 0;
    int len = (pct2 < 10) ? 1 : (pct2 < 90) ? (pct2 / 10 + 1)
                                            : 10;
    auto hit_point_bar_color = get_monster_hp_point_bar_color(monster);
    const auto &ap_r_ref = monraces_info[monster.ap_r_idx];

    // 名前
    // 表示枠に収まらない場合は途中で切る
    term_putstr(col, row + row_offset_name, max_width, TERM_WHITE, str_separate(ap_r_ref.name, max_width)[0].data());
    // HPの割合
    term_putstr(col, row + row_offset_health, max_width, TERM_WHITE, "[----------]");
    term_putstr(col + 1, row + row_offset_health, len, hit_point_bar_color, "**********");

    // 騎乗中のモンスターの状態異常は表示しない
    if (riding) {
        return;
    }

    int col_offset = 0;
    int row_offset = 0;

    // 一時的状態異常
    // MAX_WIDTHを超えたら次の行に移動する
    for (const auto &info : get_condition_layout_info(monster)) {
        if (col_offset + info.label.length() - 1 > max_width) { // 改行が必要かどうかチェック。length() - 1してるのは\0の分を文字数から取り除くため
            col_offset = 0;
            row_offset++;
        }
        term_putstr(col + col_offset, row + row_offset_condition + row_offset, max_width, info.color, info.label.data());
        col_offset += info.label.length() + 1; // 文字数と空白の分だけoffsetを加算
    }
}
