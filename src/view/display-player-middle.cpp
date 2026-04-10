#include "view/display-player-middle.h"
#include "combat/shoot.h"
#include "game-option/birth-options.h"
#include "game-option/special-options.h"
#include "inventory/inventory-slot-types.h"
#include "mind/stances-table.h"
#include "monster/monster-status.h"
#include "object-enchant/special-object-flags.h"
#include "object/tval-types.h"
#include "perception/object-perception.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/equipment-info.h"
#include "player-info/monk-data-type.h"
#include "player-info/race-types.h"
#include "player-status/player-hand-types.h"
#include "player/attack-defense-types.h"
#include "player/player-skill.h"
#include "player/player-status-flags.h"
#include "player/player-status-table.h"
#include "player/player-status.h"
#include "sv-definition/sv-bow-types.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/inner-game-data.h"
#include "system/item-entity.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "timed-effect/timed-effects.h"
#include "view/display-util.h"
#include "view/status-first-page.h"
#include "world/world-collapsion.h"
#include "world/world.h"

/*!
 * @brief プレイヤーの打撃能力修正を表示する
 * @param creature クリーチャーへの参照
 * @param hand 武器の装備部位ID
 * @param hand_entry 項目ID
 */
static void display_player_melee_bonus(CreatureEntity &creature, int hand, int hand_entry)
{
    HIT_PROB show_tohit = creature.dis_to_h[hand];
    int show_todam = creature.dis_to_d[hand];
    auto *o_ptr = creature.inventory[INVEN_MAIN_HAND + hand].get();

    if (o_ptr->is_known()) {
        show_tohit += o_ptr->to_h;
    }
    if (o_ptr->is_known()) {
        show_todam += o_ptr->to_d;
    }

    show_tohit += creature.skill_thn / BTH_PLUS_ADJ;

    const auto buf = format("(%+d,%+d)", (int)show_tohit, (int)show_todam);
    if (!has_melee_weapon(creature, INVEN_MAIN_HAND) && !has_melee_weapon(creature, INVEN_SUB_HAND)) {
        display_player_one_line(ENTRY_BARE_HAND, buf, TERM_L_BLUE);
    } else if (has_two_handed_weapons(creature)) {
        display_player_one_line(ENTRY_TWO_HANDS, buf, TERM_L_BLUE);
    } else {
        display_player_one_line(hand_entry, buf, TERM_L_BLUE);
    }
}

/*!
 * @brief 右手に比べて左手の表示ルーチンが複雑なので分離
 * @param creature クリーチャーへの参照
 */
static void display_sub_hand(CreatureEntity &creature)
{
    if (can_attack_with_sub_hand(creature)) {
        display_player_melee_bonus(creature, 1, left_hander ? ENTRY_RIGHT_HAND2 : ENTRY_LEFT_HAND2);
        return;
    }

    CreatureClass pc(creature);
    if (!pc.equals(PlayerClassType::MONK) || ((empty_hands(creature, true) & EMPTY_HAND_MAIN) == 0)) {
        return;
    }

    if (pc.monk_stance_is(MonkStanceType::NONE)) {
        display_player_one_line(ENTRY_POSTURE, _("構えなし", "none"), TERM_YELLOW);
        return;
    }

    uint stance_num = enum2i(pc.get_monk_stance()) - 1;

    if (stance_num < monk_stances.size()) {
        display_player_one_line(ENTRY_POSTURE, format(_("%sの構え", "%s form"), monk_stances[stance_num].desc), TERM_YELLOW);
    }
}

/*!
 * @brief 弓による命中率とダメージの補正を表示する
 * @param creature クリーチャーへの参照
 */
static void display_bow_hit_damage(CreatureEntity &creature)
{
    const auto &item = *creature.inventory[INVEN_BOW];
    auto show_tohit = creature.dis_to_h_b;
    auto show_todam = 0;
    if (item.is_known()) {
        show_tohit += item.to_h;
        show_todam += item.to_d;
    }

    const auto tval = item.bi_key.tval();
    const auto median_skill_exp = PlayerSkill::weapon_exp_at(PlayerSkillRank::MASTER) / 2;
    const auto &weapon_exps = creature.weapon_exp[tval];
    constexpr auto bow_magnification = 200;
    constexpr auto xbow_magnification = 400;
    if (tval == ItemKindType::NONE) {
        show_tohit += (weapon_exps[0] - median_skill_exp) / bow_magnification;
    } else {
        const auto sval = item.bi_key.sval().value();
        const auto weapon_exp = weapon_exps[sval];
        if (item.is_cross_bow()) {
            show_tohit += weapon_exp / xbow_magnification;
        } else {
            show_tohit += (weapon_exp - median_skill_exp) / bow_magnification;
        }
    }

    show_tohit += creature.skill_thb / BTH_PLUS_ADJ;
    display_player_one_line(ENTRY_SHOOT_HIT_DAM, format("(%+d,%+d)", show_tohit, show_todam), TERM_L_BLUE);
}

/*!
 * @brief 射撃武器倍率を表示する
 * @param creature クリーチャーへの参照
 */
static void display_shoot_magnification(CreatureEntity &creature)
{
    int tmul = 0;
    if (creature.inventory[INVEN_BOW]->is_valid()) {
        tmul = creature.inventory[INVEN_BOW]->get_arrow_magnification();
        if (creature.xtra_might) {
            tmul++;
        }

        tmul = tmul * (100 + (int)(adj_str_td[creature.stat_index[A_STR]]) - 128);
    }

    display_player_one_line(ENTRY_SHOOT_POWER, format("x%d.%02d", tmul / 100, tmul % 100), TERM_L_BLUE);
}

/*!
 * @brief プレイヤーの速度から表示色を決める
 * @param creature クリーチャーへの参照
 * @param base_speed プレイヤーの速度
 */
static TERM_COLOR decide_speed_color(CreatureEntity &creature, const int base_speed)
{
    TERM_COLOR attr;
    if (base_speed > 0) {
        if (!creature.riding) {
            attr = TERM_L_GREEN;
        } else {
            attr = TERM_GREEN;
        }
    } else if (base_speed == 0) {
        if (!creature.riding) {
            attr = TERM_L_BLUE;
        } else {
            attr = TERM_GREEN;
        }
    } else {
        if (!creature.riding) {
            attr = TERM_L_UMBER;
        } else {
            attr = TERM_RED;
        }
    }

    return attr;
}

/*!
 * @brief 何らかの効果による一時的な速度変化を計算する
 * @param creature クリーチャーへの参照
 * @return プレイヤーの速度
 */
static int calc_temporary_speed(CreatureEntity &creature)
{
    if (!creature.is_player()) {
        return 0;
    }
    int tmp_speed = 0;
    if (!creature.riding) {
        if (creature.is_fast()) {
            tmp_speed += 10;
        }

        if (creature.is_decelerated()) {
            tmp_speed -= 10;
        }

        if (creature.lightspeed) {
            tmp_speed = 99;
        }
    } else {
        const auto &m_ref = creature.current_floor_ptr->get_monster(creature.riding);
        if (m_ref.is_accelerated()) {
            tmp_speed += 10;
        }

        if (m_ref.is_decelerated()) {
            tmp_speed -= 10;
        }
    }

    return tmp_speed;
}

/*!
 * @brief プレイヤーの最終的な速度を表示する
 * @param creature クリーチャーへの参照
 * @param attr 表示色
 * @param base_speed プレイヤーの素の速度
 * @param tmp_speed アイテム等で一時的に変化した速度量
 */
static void display_player_speed(CreatureEntity &creature, TERM_COLOR attr, int base_speed, int tmp_speed)
{
    char buf[160];
    if (tmp_speed) {
        if (!creature.riding) {
            if (creature.lightspeed) {
                sprintf(buf, _("光速化 (+99)", "Lightspeed (+99)"));
            } else {
                sprintf(buf, "(%+d%+d)", base_speed - tmp_speed, tmp_speed);
            }
        } else {
            sprintf(buf, _("乗馬中 (%+d%+d)", "Riding (%+d%+d)"), base_speed - tmp_speed, tmp_speed);
        }

        if (tmp_speed > 0) {
            attr = TERM_YELLOW;
        } else {
            attr = TERM_VIOLET;
        }
    } else {
        if (!creature.riding) {
            sprintf(buf, "(%+d)", base_speed);
        } else {
            sprintf(buf, _("乗馬中 (%+d)", "Riding (%+d)"), base_speed);
        }
    }

    display_player_one_line(ENTRY_SPEED, buf, attr);
    display_player_one_line(ENTRY_LEVEL, format("%d", creature.level), TERM_L_GREEN);
}

/*!
 * @brief プレイヤーの現在経験値・最大経験値・次のレベルまでに必要な経験値を表示する
 * @param creature クリーチャーへの参照
 */
static void display_player_exp(CreatureEntity &creature)
{
    CreatureRace pr(&creature);
    int e = pr.equals(PlayerRaceType::ANDROID) ? ENTRY_EXP_ANDR : ENTRY_CUR_EXP;
    if (creature.exp >= creature.max_exp) {
        display_player_one_line(e, format("%ld", creature.exp), TERM_L_GREEN);
    } else {
        display_player_one_line(e, format("%ld", creature.exp), TERM_YELLOW);
    }

    if (!pr.equals(PlayerRaceType::ANDROID)) {
        display_player_one_line(ENTRY_MAX_EXP, format("%ld", creature.max_exp), TERM_L_GREEN);
    }

    e = pr.equals(PlayerRaceType::ANDROID) ? ENTRY_EXP_TO_ADV_ANDR : ENTRY_EXP_TO_ADV;

    if (creature.level >= PY_MAX_LEVEL) {
        display_player_one_line(e, "*****", TERM_L_GREEN);
    } else if (pr.equals(PlayerRaceType::ANDROID)) {
        display_player_one_line(e, format("%ld", (int32_t)(player_exp_a[creature.level - 1] * creature.expfact / 100L)), TERM_L_GREEN);
    } else {
        display_player_one_line(e, format("%ld", (int32_t)(player_exp[creature.level - 1] * creature.expfact / 100L)), TERM_L_GREEN);
    }
}

/*!
 * @brief ゲーム内の経過時間を表示する
 * @param creature クリーチャーへの参照
 * @details fmt をstring で受けているのはClang対策
 */
static void display_playtime_in_game(CreatureEntity &creature)
{
    const auto &[day, hour, min] = AngbandWorld::get_instance().extract_date_time(InnerGameData::get_instance().get_start_race());
    const auto is_days_countable = day < MAX_DAYS;
    const std::string fmt = is_days_countable ? _("%d日目 %2d:%02d", "Day %d %2d:%02d") : _("*****日目 %2d:%02d", "Day ***** %2d:%02d");
    const auto mes = is_days_countable ? format(fmt.data(), day, hour, min) : format(fmt.data(), hour, min);
    display_player_one_line(ENTRY_DAY, mes, TERM_L_GREEN);
    display_player_one_line(ENTRY_WORLD_COLLAPSE, format("%3d.%06d%%", wc_ptr->collapse_degree / 1000000, wc_ptr->collapse_degree % 1000000), TERM_L_GREEN);

    if (creature.hp >= creature.maxhp) {
        display_player_one_line(ENTRY_HP, format("%4d/%4d", creature.hp, creature.maxhp), TERM_L_GREEN);
    } else if (creature.hp > (creature.maxhp * hitpoint_warn) / 10) {
        display_player_one_line(ENTRY_HP, format("%4d/%4d", creature.hp, creature.maxhp), TERM_YELLOW);
    } else {
        display_player_one_line(ENTRY_HP, format("%4d/%4d", creature.hp, creature.maxhp), TERM_RED);
    }

    if (creature.csp >= creature.msp) {
        display_player_one_line(ENTRY_SP, format("%4d/%4d", creature.csp, creature.msp), TERM_L_GREEN);
    } else if (creature.csp > (creature.msp * mana_warn) / 10) {
        display_player_one_line(ENTRY_SP, format("%4d/%4d", creature.csp, creature.msp), TERM_YELLOW);
    } else {
        display_player_one_line(ENTRY_SP, format("%4d/%4d", creature.csp, creature.msp), TERM_RED);
    }
}

/*!
 * @brief プレイヤーステータス表示の中央部分を表示するサブルーチン
 * @param creature クリーチャーへの参照
 * Prints the following information on the screen.
 */
void display_player_middle(CreatureEntity &creature)
{
    if (creature.is_player()) {
        if (can_attack_with_main_hand(creature)) {
            display_player_melee_bonus(creature, 0, left_hander ? ENTRY_LEFT_HAND1 : ENTRY_RIGHT_HAND1);
        }

        display_sub_hand(creature);
        display_bow_hit_damage(creature);
        display_shoot_magnification(creature);
    }
    display_player_one_line(ENTRY_BASE_AC, format("[%d,%+d]", creature.dis_ac, creature.dis_to_a), TERM_L_BLUE);

    int base_speed = creature.get_speed() - 110;
    if (creature.action == ACTION_SEARCH) {
        base_speed += 10;
    }

    TERM_COLOR attr = decide_speed_color(creature, base_speed);
    int tmp_speed = calc_temporary_speed(creature);
    display_player_speed(creature, attr, base_speed, tmp_speed);
    display_player_exp(creature);
    display_player_one_line(ENTRY_GOLD, format("%ld", creature.au), TERM_L_GREEN);
    display_playtime_in_game(creature);
    display_player_one_line(ENTRY_PLAY_TIME, AngbandWorld::get_instance().format_real_playtime(), TERM_L_GREEN);
}
