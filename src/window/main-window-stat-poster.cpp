#include "window/main-window-stat-poster.h"
#include "io/input-key-requester.h"
#include "mind/stances-table.h"
#include "monster/monster-status.h"
#include "player-base/player-class.h"
#include "player-info/bluemage-data-type.h"
#include "player-info/mane-data-type.h"
#include "player-info/monk-data-type.h"
#include "player-info/ninja-data-type.h"
#include "player-info/samurai-data-type.h"
#include "player-info/sniper-data-type.h"
#include "player/attack-defense-types.h"
#include "player/digestion-processor.h"
#include "player/player-realm.h"
#include "player/player-status-table.h"
#include "player/player-status.h"
#include "realm/realm-hex-numbers.h"
#include "realm/realm-types.h"
#include "spell-realm/spells-hex.h"
#include "status/element-resistance.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/monster-entity.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "timed-effect/timed-effects.h"
#include "view/status-bars-table.h"
#include "window/main-window-row-column.h"
#include "world/world.h"

/*!
 * @brief 32ビット変数配列の指定位置のビットフラグを1にする。
 * @param FLG フラグ位置(ビット)
 */
#define ADD_BAR_FLAG(FLG) (bar_flags[FLG / 32] |= (1UL << (FLG % 32)))

/*!
 * @brief 32ビット変数配列の指定位置のビットフラグが1かどうかを返す。
 * @param FLG フラグ位置(ビット)
 * @return 1ならば0以外を返す
 */
#define IS_BAR_FLAG(FLG) (bar_flags[FLG / 32] & (1UL << (FLG % 32)))

/*!
 * @brief プレイヤー能力値を描画する / Print character stat in given row, column
 * @param stat 描画するステータスのID
 */
void print_stat(CreatureEntity &creature, int stat)
{
    if (creature.stat_cur[stat] < creature.stat_max[stat]) {
        put_str(stat_names_reduced[stat], ROW_STAT + stat, 0);
        c_put_str(TERM_YELLOW, cnv_stat(creature.stat_use[stat]), ROW_STAT + stat, COL_STAT + 6);
    } else {
        put_str(stat_names[stat], ROW_STAT + stat, 0);
        c_put_str(TERM_L_GREEN, cnv_stat(creature.stat_use[stat]), ROW_STAT + stat, COL_STAT + 6);
    }

    if (creature.stat_max[stat] != creature.stat_max_max[stat]) {
        return;
    }

#ifdef JP
    /* 日本語にかぶらないように表示位置を変更 */
    put_str("!", ROW_STAT + stat, 5);
#else
    put_str("!", ROW_STAT + stat, 3);
#endif
}

/*!
 * @brief プレイヤーの負傷状態を表示する
 */
void print_cut(CreatureEntity &creature)
{
    const auto &player_cut = creature.effects()->cut();
    if (!player_cut.is_cut()) {
        put_str("            ", ROW_CUT, COL_CUT);
        return;
    }

    auto [color, stat] = player_cut.get_expr();
    c_put_str(color, stat, ROW_CUT, COL_CUT);
}

/*!
 * @brief プレイヤーの朦朧状態を表示する
 * @param creature クリーチャーへの参照
 */
void print_stun(CreatureEntity &creature)
{
    const auto &player_stun = creature.effects()->stun();
    if (!player_stun.is_stunned()) {
        put_str("            ", ROW_STUN, COL_STUN);
        return;
    }

    const auto &[color, stat] = player_stun.get_expr();
    c_put_str(color, stat, ROW_STUN, COL_STUN);
}

/*!
 * @brief プレイヤーの空腹状態を表示する / Prints status of hunger
 * @param creature クリーチャーへの参照
 */
void print_hunger(CreatureEntity &creature)
{
    if (AngbandWorld::get_instance().wizard && creature.current_floor_ptr->inside_arena) {
        return;
    }

    const auto [width, height] = term_get_size();
    const auto row = height + ROW_HUNGRY;

    if (creature.food < PY_FOOD_FAINT) {
        c_put_str(TERM_RED, _("衰弱  ", "Weak  "), row, COL_HUNGRY);
        return;
    }

    if (creature.food < PY_FOOD_WEAK) {
        c_put_str(TERM_ORANGE, _("衰弱  ", "Weak  "), row, COL_HUNGRY);
        return;
    }

    if (creature.food < PY_FOOD_ALERT) {
        c_put_str(TERM_YELLOW, _("空腹  ", "Hungry"), row, COL_HUNGRY);
        return;
    }

    if (creature.food < PY_FOOD_FULL) {
        c_put_str(TERM_L_GREEN, "      ", row, COL_HUNGRY);
        return;
    }

    if (creature.food < PY_FOOD_MAX) {
        c_put_str(TERM_L_GREEN, _("満腹  ", "Full  "), row, COL_HUNGRY);
        return;
    }

    c_put_str(TERM_GREEN, _("食過ぎ", "Gorged"), row, COL_HUNGRY);
}

/*!
 * @brief プレイヤーの行動状態を表示する / Prints Searching, Resting, Paralysis, or 'count' status
 * @param creature クリーチャーへの参照
 * @details
 * Display is always exactly 10 characters wide (see below)
 * This function was a major bottleneck when resting, so a lot of
 * the text formatting code was optimized in place below.
 */
void print_state(CreatureEntity &creature)
{
    TERM_COLOR attr = TERM_WHITE;
    std::string text;
    if (command_rep) {
        if (command_rep > 999) {
            text = format("%2d00", command_rep / 100);
        } else {
            text = format("  %2d", command_rep);
        }

        c_put_str(attr, format("%5.5s", text.data()), ROW_STATE, COL_STATE);
        return;
    }

    switch (creature.action) {
    case ACTION_SEARCH: {
        text = _("探索", "Sear");
        break;
    }
    case ACTION_REST:
        if (creature.resting > 0) {
            text = format("%4d", creature.resting);
        } else if (creature.resting == COMMAND_ARG_REST_FULL_HEALING) {
            text = "****";
        } else if (creature.resting == COMMAND_ARG_REST_UNTIL_DONE) {
            text = "&&&&";
        } else {
            text = "    ";
        }

        break;

    case ACTION_LEARN: {
        text = _("学習", "lear");
        auto bluemage_data = CreatureClass(creature).get_specific_data<bluemage_data_type>();
        if (bluemage_data->new_magic_learned) {
            attr = TERM_L_RED;
        }
        break;
    }
    case ACTION_FISH: {
        text = _("釣り", "fish");
        break;
    }
    case ACTION_MONK_STANCE: {
        if (auto stance = CreatureClass(creature).get_monk_stance();
            stance != MonkStanceType::NONE) {
            switch (stance) {
            case MonkStanceType::GENBU:
                attr = TERM_GREEN;
                break;
            case MonkStanceType::BYAKKO:
                attr = TERM_WHITE;
                break;
            case MonkStanceType::SEIRYU:
                attr = TERM_L_BLUE;
                break;
            case MonkStanceType::SUZAKU:
                attr = TERM_L_RED;
                break;
            default:
                break;
            }
            text = monk_stances[enum2i(stance) - 1].desc;
        }
        break;
    }
    case ACTION_SAMURAI_STANCE: {
        if (auto stance = CreatureClass(creature).get_samurai_stance();
            stance != SamuraiStanceType::NONE) {
            text = samurai_stances[enum2i(stance) - 1].desc;
        }
        break;
    }
    case ACTION_SING: {
        text = _("歌  ", "Sing");
        break;
    }
    case ACTION_HAYAGAKE: {
        text = _("速駆", "Fast");
        break;
    }
    case ACTION_SPELL: {
        text = _("詠唱", "Spel");
        break;
    }
    default: {
        text = "    ";
        break;
    }
    }

    c_put_str(attr, format("%5.5s", text.data()), ROW_STATE, COL_STATE);
}

/*!
 * @brief プレイヤーの行動速度を表示する / Prints the speed_value of a character.			-CJS-
 * @param creature クリーチャーへの参照
 */
void print_speed(CreatureEntity &creature)
{
    const auto &[wid, hgt] = term_get_size();
    auto col_speed = wid + COL_SPEED;
    auto row_speed = hgt + ROW_SPEED;

    const auto speed = creature.get_speed() - STANDARD_SPEED;
    const auto &floor = *creature.current_floor_ptr;
    bool is_player_fast = creature.is_fast();
    char buf[32] = "";
    TERM_COLOR attr = TERM_WHITE;
    const auto is_slow = creature.effects()->deceleration().is_slow();
    if (speed > 0) {
        if (creature.riding) {
            const auto &monster = floor.m_list[creature.riding];
            if (monster.is_accelerated() && !monster.is_decelerated()) {
                attr = TERM_L_BLUE;
            } else if (monster.is_decelerated() && !monster.is_accelerated()) {
                attr = TERM_VIOLET;
            } else {
                attr = TERM_GREEN;
            }
        } else if ((is_player_fast && !is_slow) || creature.lightspeed) {
            attr = TERM_YELLOW;
        } else if (is_slow && !is_player_fast) {
            attr = TERM_VIOLET;
        } else {
            attr = TERM_L_GREEN;
        }
        sprintf(buf, "%s(+%d)", (creature.riding ? _("乗馬", "Ride") : _("加速", "Fast")), speed);
    } else if (speed < 0) {
        if (creature.riding) {
            const auto &monster = floor.m_list[creature.riding];
            if (monster.is_accelerated() && !monster.is_decelerated()) {
                attr = TERM_L_BLUE;
            } else if (monster.is_decelerated() && !monster.is_accelerated()) {
                attr = TERM_VIOLET;
            } else {
                attr = TERM_RED;
            }
        } else if (is_player_fast && !is_slow) {
            attr = TERM_YELLOW;
        } else if (is_slow && !is_player_fast) {
            attr = TERM_VIOLET;
        } else {
            attr = TERM_L_UMBER;
        }
        sprintf(buf, "%s(%d)", (creature.riding ? _("乗馬", "Ride") : _("減速", "Slow")), speed);
    } else if (creature.riding) {
        attr = TERM_GREEN;
        strcpy(buf, _("乗馬中", "Riding"));
    }

    c_put_str(attr, format("%-9s", buf), row_speed, col_speed);
}

/*!
 * @brief プレイヤーの呪文学習可能状態を表示する
 * @param creature クリーチャーへの参照
 */
void print_study(CreatureEntity &creature)
{
    const auto &[wid, hgt] = term_get_size();
    const auto col_study = wid + COL_STUDY;
    const auto row_study = hgt + ROW_STUDY;
    if (creature.new_spells) {
        put_str(_("学習", "Stud"), row_study, col_study);
    } else {
        put_str("    ", row_study, col_study);
    }
}

/*!
 * @brief プレイヤーのものまね可能状態を表示する
 * @param creature クリーチャーへの参照
 */
void print_imitation(CreatureEntity &creature)
{
    const auto &[wid, hgt] = term_get_size();
    const auto col_study = wid + COL_STUDY;
    const auto row_study = hgt + ROW_STUDY;
    CreatureClass pc(creature);
    if (!pc.equals(PlayerClassType::IMITATOR)) {
        return;
    }

    auto mane_data = pc.get_specific_data<mane_data_type>();

    if (mane_data->mane_list.size() == 0) {
        put_str("    ", row_study, col_study);
        return;
    }

    TERM_COLOR attr = mane_data->new_mane ? TERM_L_RED : TERM_WHITE;
    c_put_str(attr, _("まね", "Imit"), row_study, col_study);
}

/*!
 * @brief 画面下部に表示すべき呪術の呪文をリストアップする
 * @param creature クリーチャーへの参照
 * @bar_flags 表示可否を決めるためのフラグ群
 */
static void add_hex_status_flags(CreatureEntity &creature, BIT_FLAGS *bar_flags)
{
    if (!PlayerRealm(creature).is_realm_hex()) {
        return;
    }

    SpellHex spell_hex(creature);
    if (spell_hex.is_spelling_specific(HEX_BLESS)) {
        ADD_BAR_FLAG(BAR_BLESSED);
    }

    if (spell_hex.is_spelling_specific(HEX_DEMON_AURA)) {
        ADD_BAR_FLAG(BAR_SHFIRE);
        ADD_BAR_FLAG(BAR_REGENERATION);
    }

    if (spell_hex.is_spelling_specific(HEX_XTRA_MIGHT)) {
        ADD_BAR_FLAG(BAR_MIGHT);
    }

    if (spell_hex.is_spelling_specific(HEX_DETECT_EVIL)) {
        ADD_BAR_FLAG(BAR_ESP_EVIL);
    }

    if (spell_hex.is_spelling_specific(HEX_ICE_ARMOR)) {
        ADD_BAR_FLAG(BAR_SHCOLD);
    }

    if (spell_hex.is_spelling_specific(HEX_RUNESWORD)) {
        ADD_BAR_FLAG(BAR_RUNESWORD);
    }

    if (spell_hex.is_spelling_specific(HEX_BUILDING)) {
        ADD_BAR_FLAG(BAR_BUILD);
    }

    if (spell_hex.is_spelling_specific(HEX_ANTI_TELE)) {
        ADD_BAR_FLAG(BAR_ANTITELE);
    }

    if (spell_hex.is_spelling_specific(HEX_SHOCK_CLOAK)) {
        ADD_BAR_FLAG(BAR_SHELEC);
    }

    if (spell_hex.is_spelling_specific(HEX_SHADOW_CLOAK)) {
        ADD_BAR_FLAG(BAR_SHSHADOW);
    }

    if (spell_hex.is_spelling_specific(HEX_CONFUSION)) {
        ADD_BAR_FLAG(BAR_ATTKCONF);
    }

    if (spell_hex.is_spelling_specific(HEX_EYE_FOR_EYE)) {
        ADD_BAR_FLAG(BAR_EYEEYE);
    }

    if (spell_hex.is_spelling_specific(HEX_ANTI_MULTI)) {
        ADD_BAR_FLAG(BAR_ANTIMULTI);
    }

    if (spell_hex.is_spelling_specific(HEX_VAMP_BLADE)) {
        ADD_BAR_FLAG(BAR_VAMPILIC);
    }

    if (spell_hex.is_spelling_specific(HEX_ANTI_MAGIC)) {
        ADD_BAR_FLAG(BAR_ANTIMAGIC);
    }

    if (spell_hex.is_spelling_specific(HEX_CURE_LIGHT) || spell_hex.is_spelling_specific(HEX_CURE_SERIOUS) || spell_hex.is_spelling_specific(HEX_CURE_CRITICAL)) {
        ADD_BAR_FLAG(BAR_CURE);
    }

    if (spell_hex.get_revenge_turn() > 0) {
        auto revenge_type = spell_hex.get_revenge_type();
        if (revenge_type == SpellHexRevengeType::PATIENCE) {
            ADD_BAR_FLAG(BAR_PATIENCE);
        }

        if (revenge_type == SpellHexRevengeType::REVENGE) {
            ADD_BAR_FLAG(BAR_REVENGE);
        }
    }
}

/*!
 * @brief 下部に状態表示を行う / Show status bar
 */
void print_status(CreatureEntity &creature)
{
    const auto &[wid, hgt] = term_get_size();
    const auto row_statbar = hgt + ROW_STATBAR;
    const auto max_col_statbar = wid + MAX_COL_STATBAR;
    term_erase(0, row_statbar, max_col_statbar);
    BIT_FLAGS bar_flags[3]{};
    auto effects = creature.effects();
    if (creature.tsuyoshi) {
        ADD_BAR_FLAG(BAR_TSUYOSHI);
    }

    if (effects->hallucination().is_hallucinated()) {
        ADD_BAR_FLAG(BAR_HALLUCINATION);
    }

    if (creature.is_blind()) {
        ADD_BAR_FLAG(BAR_BLINDNESS);
    }

    if (effects->paralysis().is_paralyzed()) {
        ADD_BAR_FLAG(BAR_PARALYZE);
    }

    if (effects->confusion().is_confused()) {
        ADD_BAR_FLAG(BAR_CONFUSE);
    }

    if (effects->poison().is_poisoned()) {
        ADD_BAR_FLAG(BAR_POISONED);
    }

    if (creature.tim_invis) {
        ADD_BAR_FLAG(BAR_SENSEUNSEEN);
    }

    auto sniper_data = CreatureClass(creature).get_specific_data<SniperData>();
    if (sniper_data && (sniper_data->concent >= CONCENT_RADAR_THRESHOLD)) {
        ADD_BAR_FLAG(BAR_SENSEUNSEEN);
        ADD_BAR_FLAG(BAR_NIGHTSIGHT);
    }

    if (is_time_limit_esp(creature)) {
        ADD_BAR_FLAG(BAR_TELEPATHY);
    }

    if (creature.tim_regen) {
        ADD_BAR_FLAG(BAR_REGENERATION);
    }

    if (creature.tim_infra) {
        ADD_BAR_FLAG(BAR_INFRAVISION);
    }

    if (effects->protection().is_protected()) {
        ADD_BAR_FLAG(BAR_PROTEVIL);
    }

    if (creature.is_invulnerable()) {
        ADD_BAR_FLAG(BAR_INVULN);
    }

    if (creature.wraith_form) {
        ADD_BAR_FLAG(BAR_WRAITH);
    }

    if (creature.tim_pass_wall) {
        ADD_BAR_FLAG(BAR_PASSWALL);
    }

    if (creature.tim_reflect) {
        ADD_BAR_FLAG(BAR_REFLECTION);
    }

    if (creature.is_hero()) {
        ADD_BAR_FLAG(BAR_HEROISM);
    }

    if (creature.is_shero()) {
        ADD_BAR_FLAG(BAR_BERSERK);
    }

    if (creature.is_blessed()) {
        ADD_BAR_FLAG(BAR_BLESSED);
    }

    if (creature.magicdef) {
        ADD_BAR_FLAG(BAR_MAGICDEFENSE);
    }

    if (creature.tsubureru) {
        ADD_BAR_FLAG(BAR_EXPAND);
    }

    if (creature.shield) {
        ADD_BAR_FLAG(BAR_STONESKIN);
    }

    auto ninja_data = CreatureClass(creature).get_specific_data<ninja_data_type>();
    if (ninja_data && ninja_data->kawarimi) {
        ADD_BAR_FLAG(BAR_KAWARIMI);
    }

    if (creature.special_defense & DEFENSE_ACID) {
        ADD_BAR_FLAG(BAR_IMMACID);
    }

    if (is_oppose_acid(creature)) {
        ADD_BAR_FLAG(BAR_RESACID);
    }

    if (creature.special_defense & DEFENSE_ELEC) {
        ADD_BAR_FLAG(BAR_IMMELEC);
    }

    if (is_oppose_elec(creature)) {
        ADD_BAR_FLAG(BAR_RESELEC);
    }

    if (creature.special_defense & DEFENSE_FIRE) {
        ADD_BAR_FLAG(BAR_IMMFIRE);
    }

    if (is_oppose_fire(creature)) {
        ADD_BAR_FLAG(BAR_RESFIRE);
    }

    if (creature.special_defense & DEFENSE_COLD) {
        ADD_BAR_FLAG(BAR_IMMCOLD);
    }

    if (is_oppose_cold(creature)) {
        ADD_BAR_FLAG(BAR_RESCOLD);
    }

    if (is_oppose_pois(creature)) {
        ADD_BAR_FLAG(BAR_RESPOIS);
    }

    if (creature.word_recall) {
        ADD_BAR_FLAG(BAR_RECALL);
    }

    if (creature.alter_reality) {
        ADD_BAR_FLAG(BAR_ALTER);
    }

    if (effects->fear().is_fearful()) {
        ADD_BAR_FLAG(BAR_AFRAID);
    }

    if (creature.tim_res_time) {
        ADD_BAR_FLAG(BAR_RESTIME);
    }

    if (creature.multishadow) {
        ADD_BAR_FLAG(BAR_MULTISHADOW);
    }

    if (creature.special_attack & ATTACK_CONFUSE) {
        ADD_BAR_FLAG(BAR_ATTKCONF);
    }

    if (creature.resist_magic) {
        ADD_BAR_FLAG(BAR_REGMAGIC);
    }

    if (creature.ult_res) {
        ADD_BAR_FLAG(BAR_ULTIMATE);
    }

    if (creature.tim_levitation) {
        ADD_BAR_FLAG(BAR_LEVITATE);
    }

    if (creature.tim_res_nether) {
        ADD_BAR_FLAG(BAR_RESNETH);
    }

    if (creature.dustrobe) {
        ADD_BAR_FLAG(BAR_DUSTROBE);
    }

    if (creature.special_attack & ATTACK_FIRE) {
        ADD_BAR_FLAG(BAR_ATTKFIRE);
    }

    if (creature.special_attack & ATTACK_COLD) {
        ADD_BAR_FLAG(BAR_ATTKCOLD);
    }

    if (creature.special_attack & ATTACK_ELEC) {
        ADD_BAR_FLAG(BAR_ATTKELEC);
    }

    if (creature.special_attack & ATTACK_ACID) {
        ADD_BAR_FLAG(BAR_ATTKACID);
    }

    if (creature.special_attack & ATTACK_POIS) {
        ADD_BAR_FLAG(BAR_ATTKPOIS);
    }

    if (ninja_data && ninja_data->s_stealth) {
        ADD_BAR_FLAG(BAR_SUPERSTEALTH);
    }

    if (creature.tim_sh_fire) {
        ADD_BAR_FLAG(BAR_SHFIRE);
    }

    if (is_time_limit_stealth(creature)) {
        ADD_BAR_FLAG(BAR_STEALTH);
    }

    if (creature.tim_sh_touki) {
        ADD_BAR_FLAG(BAR_TOUKI);
    }

    if (creature.tim_sh_holy) {
        ADD_BAR_FLAG(BAR_SHHOLY);
    }

    if (creature.tim_eyeeye) {
        ADD_BAR_FLAG(BAR_EYEEYE);
    }

    if (creature.tim_res_lite) {
        ADD_BAR_FLAG(BAR_RESLITE);
    }

    if (creature.tim_res_dark) {
        ADD_BAR_FLAG(BAR_RESDARK);
    }

    if (creature.tim_res_fear) {
        ADD_BAR_FLAG(BAR_RESFEAR);
    }

    if (creature.tim_emission) {
        ADD_BAR_FLAG(BAR_EMISSION);
    }

    if (creature.tim_exorcism) {
        ADD_BAR_FLAG(BAR_EXORCISM);
    }

    if (creature.tim_imm_dark) {
        ADD_BAR_FLAG(BAR_IMMDARK);
    }

    add_hex_status_flags(creature, bar_flags);
    TERM_LEN col = 0, num = 0;
    for (int i = 0; stat_bars[i].sstr; i++) {
        if (IS_BAR_FLAG(i)) {
            col += strlen(stat_bars[i].lstr) + 1;
            num++;
        }
    }

    int space = 2;
    if (col - 1 > max_col_statbar) {
        space = 0;
        col = 0;

        for (int i = 0; stat_bars[i].sstr; i++) {
            if (IS_BAR_FLAG(i)) {
                col += strlen(stat_bars[i].sstr);
            }
        }

        if (col - 1 <= max_col_statbar - (num - 1)) {
            space = 1;
            col += num - 1;
        }
    }

    col = (max_col_statbar - col) / 2;
    for (int i = 0; stat_bars[i].sstr; i++) {
        if (!IS_BAR_FLAG(i)) {
            continue;
        }

        concptr str;
        if (space == 2) {
            str = stat_bars[i].lstr;
        } else {
            str = stat_bars[i].sstr;
        }

        c_put_str(stat_bars[i].attr, str, row_statbar, col);
        col += strlen(str);
        if (space > 0) {
            col++;
        }

        if (col > max_col_statbar) {
            break;
        }
    }
}

/*!
 * @brief プレイヤーのステータスを一括表示する（下部分） / Display extra info (mostly below map)
 * @param creature クリーチャーへの参照
 */
void print_frame_extra(CreatureEntity &creature)
{
    print_cut(creature);
    print_stun(creature);
    print_hunger(creature);
    print_state(creature);
    print_speed(creature);
    print_study(creature);
    print_imitation(creature);
    print_status(creature);
}
