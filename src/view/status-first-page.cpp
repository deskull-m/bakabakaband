/*!
 * @file status-first-page.c
 * @brief キャラ基本情報及び技能値の表示
 * @date 2020/02/23
 * @author Hourier
 */

#include "view/status-first-page.h"
#include "artifact/fixed-art-types.h"
#include "combat/attack-power-table.h"
#include "combat/shoot.h"
#include "game-option/text-display-options.h"
#include "hpmp/hp-mp-regenerator.h"
#include "inventory/inventory-slot-types.h"
#include "mind/monk-attack.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object/tval-types.h"
#include "perception/object-perception.h"
#include "pet/pet-util.h"
#include "player-base/player-class.h"
#include "player-info/equipment-info.h"
#include "player-info/monk-data-type.h"
#include "player-info/samurai-data-type.h"
#include "player-status/player-hand-types.h"
#include "player/attack-defense-types.h"
#include "player/digestion-processor.h"
#include "player/player-status-flags.h"
#include "player/player-status.h"
#include "player/special-defense-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/terrain/terrain-definition.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "view/display-util.h"

/*!
 * @brief
 * @param creature クリーチャーへの参照
 * @param o_ptr 装備中の弓への参照ポインタ
 * @param shots 射撃回数
 * @param shot_frac 射撃速度
 */
static void calc_shot_params(CreatureEntity &creature, ItemEntity *o_ptr, int *shots, int *shot_frac)
{
    if (!o_ptr->is_valid()) {
        return;
    }

    const auto energy_fire = o_ptr->get_bow_energy();
    *shots = creature.num_fire * 100;
    *shot_frac = ((*shots) * 100 / energy_fire) % 100;
    *shots = (*shots) / energy_fire;
    if (!o_ptr->is_specific_artifact(FixedArtifactId::CRIMSON)) {
        return;
    }

    *shots = 1;
    *shot_frac = 0;
    if (!CreatureClass(creature).equals(PlayerClassType::ARCHER)) {
        return;
    }

    if (creature.level >= 10) {
        (*shots)++;
    }
    if (creature.level >= 30) {
        (*shots)++;
    }
    if (creature.level >= 45) {
        (*shots)++;
    }
}

/*!
 * @brief 武器装備に制限のあるクラスで、直接攻撃のダメージを計算する
 * @param creature クリーチャーへの参照
 * @param hand 手 (利き手が0、反対の手が1…のはず)
 * @param damage 直接攻撃のダメージ
 * @param basedam 素手における直接攻撃のダメージ
 * @param o_ptr 装備中の武器への参照ポインタ
 * @return 利き手ならTRUE、反対の手ならFALSE
 */
static bool calc_weapon_damage_limit(CreatureEntity &creature, int hand, int *damage, int *basedam, ItemEntity *o_ptr)
{
    PLAYER_LEVEL level = creature.level;
    if (hand > 0) {
        damage[hand] = 0;
        return false;
    }

    CreatureClass pc(creature);
    if (pc.equals(PlayerClassType::FORCETRAINER)) {
        level = std::max<short>(1, level - 3);
    }

    if (pc.monk_stance_is(MonkStanceType::BYAKKO)) {
        *basedam = monk_ave_damage[level][1];
    } else if (pc.monk_stance_is(MonkStanceType::GENBU) || pc.monk_stance_is(MonkStanceType::SUZAKU)) {
        *basedam = monk_ave_damage[level][2];
    } else {
        *basedam = monk_ave_damage[level][0];
    }
    bool impact = creature.impact != 0;
    WEIGHT weight = creature.level * calc_monk_attack_weight(creature);
    int to_h = creature.level * 7 / 10; // 命中計算が煩雑なのでおよその値を使用する

    *basedam = calc_expect_crit(creature, weight, to_h, *basedam, creature.to_h[0], false, impact, 100);

    damage[hand] += *basedam;
    if (o_ptr->bi_key == BaseitemKey(ItemKindType::SWORD, SV_POISON_NEEDLE)) {
        damage[hand] = 1;
    }
    if (damage[hand] < 0) {
        damage[hand] = 0;
    }

    return true;
}

/*!
 * @brief 片手あたりのダメージ量を計算する
 * @param o_ptr 装備中の武器への参照ポインタ
 * @param hand 手
 * @param damage 直接攻撃のダメージ
 * @param basedam 素手における直接攻撃のダメージ
 * @return 素手ならFALSE、武器を持っていればTRUE
 */
static bool calc_weapon_one_hand(ItemEntity *o_ptr, int hand, int *damage, int *basedam)
{
    if (!o_ptr->is_valid()) {
        return false;
    }

    *basedam = 0;
    damage[hand] += *basedam;
    if (o_ptr->bi_key == BaseitemKey(ItemKindType::SWORD, SV_POISON_NEEDLE)) {
        damage[hand] = 1;
    }

    if (damage[hand] < 0) {
        damage[hand] = 0;
    }

    return true;
}

/*!
 * @brief 技能ランクの表示基準を定める
 * Returns a "rating" of x depending on y
 * @param x 技能値
 * @param y 技能値に対するランク基準比
 * @return スキル レベルのテキスト説明とその説明のカラー インデックスのペア
 */
static std::pair<std::string, TERM_COLOR> likert(int x, int y)
{
    std::string desc;

    if (show_actual_value) {
        desc = format("%3d-", x);
    }

    if (x < 0) {
        return make_pair(desc.append(_("最低", "Very Bad")), TERM_L_DARK);
    }

    if (y <= 0) {
        y = 1;
    }

    switch ((x / y)) {
    case 0:
    case 1: {
        return make_pair(desc.append(_("悪い", "Bad")), TERM_RED);
    }
    case 2: {
        return make_pair(desc.append(_("劣る", "Poor")), TERM_L_RED);
    }
    case 3:
    case 4: {
        return make_pair(desc.append(_("普通", "Fair")), TERM_ORANGE);
    }
    case 5: {
        return make_pair(desc.append(_("良い", "Good")), TERM_YELLOW);
    }
    case 6: {
        return make_pair(desc.append(_("大変良い", "Very Good")), TERM_YELLOW);
    }
    case 7:
    case 8: {
        return make_pair(desc.append(_("卓越", "Excellent")), TERM_L_GREEN);
    }
    case 9:
    case 10:
    case 11:
    case 12:
    case 13: {
        return make_pair(desc.append(_("超越", "Superb")), TERM_GREEN);
    }
    case 14:
    case 15:
    case 16:
    case 17: {
        return make_pair(desc.append(_("英雄的", "Heroic")), TERM_BLUE);
    }
    default: {
        desc.append(format(_("伝説的[%d]", "Legendary[%d]"), (int)((((x / y) - 17) * 5) / 2)));
        return make_pair(desc, TERM_VIOLET);
    }
    }
}

/*!
 * @brief 弓＋両手の武器それぞれについてダメージを計算する
 * @param creature クリーチャーへの参照
 * @param damage 直接攻撃のダメージ
 * @param to_h 命中補正
 */
static void calc_two_hands(CreatureEntity &creature, int *damage, int *to_h)
{
    ItemEntity *o_ptr;
    o_ptr = creature.inventory[INVEN_BOW].get();

    for (int i = 0; i < 2; i++) {
        int basedam;
        damage[i] = creature.dis_to_d[i] * 100;
        CreatureClass pc(creature);
        if (pc.is_martial_arts_pro() && (empty_hands(creature, true) & EMPTY_HAND_MAIN)) {
            if (!calc_weapon_damage_limit(creature, i, damage, &basedam, o_ptr)) {
                break;
            }

            continue;
        }

        o_ptr = creature.inventory[INVEN_MAIN_HAND + i].get();
        if (!calc_weapon_one_hand(o_ptr, i, damage, &basedam)) {
            continue;
        }

        to_h[i] = 0;

        if (o_ptr->is_known()) {
            damage[i] += o_ptr->to_d * 100;
            to_h[i] += o_ptr->to_h;
        }
        auto &player_ptr = creature;
        const auto mindice = (o_ptr->damage_dice.num + player_ptr.damage_dice_bonus[i].num);
        const auto maxdice = mindice * (o_ptr->damage_dice.sides + player_ptr.damage_dice_bonus[i].sides);

        basedam = calc_expect_dice(player_ptr, mindice, creature.to_h[i], o_ptr);
        basedam += calc_expect_dice(player_ptr, maxdice, creature.to_h[i], o_ptr);
        damage[i] += basedam * 50; // x100 for display

        if (o_ptr->bi_key == BaseitemKey(ItemKindType::SWORD, SV_POISON_NEEDLE)) {
            damage[i] = 1;
        }
        if (damage[i] < 0) {
            damage[i] = 0;
        }
    }
}

/*!
 * @brief HP回復量/ターンを計算する
 * @param creature クリーチャーへの参照
 * @return 100倍したHP回復量（小数第2位まで表示するため）
 */
static int calculate_hp_regen_rate(CreatureEntity &creature)
{
    CreatureClass pc(creature);
    if (pc.samurai_stance_is(SamuraiStanceType::KOUKIJIN)) {
        return 0;
    }
    if (creature.action == ACTION_HAYAGAKE) {
        return 0;
    }

    int regen_amount = PY_REGEN_NORMAL;

    // 満腹度による補正
    if (creature.food < PY_FOOD_WEAK) {
        if (creature.food < PY_FOOD_STARVE) {
            regen_amount = 0;
        } else if (creature.food < PY_FOOD_FAINT) {
            regen_amount = PY_REGEN_FAINT;
        } else {
            regen_amount = PY_REGEN_WEAK;
        }
    }

    // 毒・切り傷で回復しない
    const auto effects = creature.effects();
    if (effects->poison().is_poisoned() || effects->cut().is_cut()) {
        regen_amount = 0;
    }

    // 再生能力
    if (creature.regenerate) {
        regen_amount = regen_amount * 2;
    }

    // 構え・型による補正
    if (!pc.monk_stance_is(MonkStanceType::NONE) || !pc.samurai_stance_is(SamuraiStanceType::NONE)) {
        regen_amount /= 2;
    }

    // 呪いによる補正
    if (creature.cursed.has(CurseTraitType::SLOW_REGEN)) {
        regen_amount /= 5;
    }

    // 探索・休息中は2倍
    if ((creature.action == ACTION_SEARCH) || (creature.action == ACTION_REST)) {
        regen_amount = regen_amount * 2;
    }

    // 地形による衛生補正
    const auto &floor = *creature.current_floor_ptr;
    const auto &grid = floor.get_grid(creature.get_position());
    const auto &terrain = grid.get_terrain();
    if (regen_amount > 0 && terrain.hygiene != 0) {
        const int hygiene_modifier = 100 + terrain.hygiene;
        regen_amount = (regen_amount * hygiene_modifier) / 100;
        if (regen_amount < 0) {
            regen_amount = 0;
        }
    }

    // ミュータント補正
    regen_amount = (regen_amount * creature.mutant_regenerate_mod) / 100;

    // 実際の回復量を計算 (10ターンごとに処理されるので1ターンあたりの量に変換)
    // percent = regen_amount は 1/2^16 単位なので、実際のHP回復量は:
    // (maxhp * regen_amount + PY_REGEN_HPBASE) >> 16
    // これも10で割って1ターンあたりにし、さらに100ターン分に変換
    int64_t hp_per_turn = ((int64_t)creature.maxhp * regen_amount + PY_REGEN_HPBASE) * 10000 >> 16;

    return static_cast<int>(hp_per_turn);
}

/*!
 * @brief MP回復量/ターンを計算する
 * @param creature クリーチャーへの参照
 * @return 100倍したMP回復量（小数第2位まで表示するため）
 */
static int calculate_mp_regen_rate(CreatureEntity &creature)
{
    int regen_amount = PY_REGEN_NORMAL;

    // 満腹度による補正
    if (creature.food < PY_FOOD_WEAK) {
        if (creature.food < PY_FOOD_STARVE) {
            regen_amount = 0;
        } else if (creature.food < PY_FOOD_FAINT) {
            regen_amount = PY_REGEN_FAINT;
        } else {
            regen_amount = PY_REGEN_WEAK;
        }
    }

    // 毒で回復しない
    const auto effects = creature.effects();
    if (effects->poison().is_poisoned()) {
        regen_amount = 0;
    }

    // 再生能力
    if (creature.regenerate) {
        regen_amount = regen_amount * 2;
    }

    // 構え・型による補正
    CreatureClass pc(creature);
    if (!pc.monk_stance_is(MonkStanceType::NONE) || !pc.samurai_stance_is(SamuraiStanceType::NONE)) {
        regen_amount /= 2;
    }

    // 呪いによる補正
    if (creature.cursed.has(CurseTraitType::SLOW_REGEN)) {
        regen_amount /= 5;
    }

    // 探索・休息中は2倍
    if ((creature.action == ACTION_SEARCH) || (creature.action == ACTION_REST)) {
        regen_amount = regen_amount * 2;
    }

    // ペットの維持コスト
    int upkeep_factor = calculate_upkeep(creature);
    if ((creature.action == ACTION_LEARN) || (creature.action == ACTION_HAYAGAKE) || pc.samurai_stance_is(SamuraiStanceType::KOUKIJIN)) {
        upkeep_factor += 100;
    }

    // 回復率を計算 (100分率)
    int32_t regen_rate = regen_amount * 100 - upkeep_factor * PY_REGEN_NORMAL;

    // マイナスの場合は回復しない（減少する）
    if (regen_rate < 0) {
        // 減少量を計算 (表示上はマイナス表示)
        int64_t mp_decay_per_10turn = ((int64_t)creature.msp * (-regen_rate) / 100 + PY_REGEN_MNBASE) >> 16;
        int64_t mp_per_turn_x100 = (mp_decay_per_10turn * 100) / 10;
        int64_t mp_per_100turn = -(mp_per_turn_x100 * 100); // 100ターン分（マイナス）
        return static_cast<int>(mp_per_100turn);
    }

    // 実際の回復量を計算
    int64_t mp_per_10turn = ((int64_t)creature.msp * regen_rate / 100 + PY_REGEN_MNBASE) >> 16;
    int64_t mp_per_turn_x100 = (mp_per_10turn * 100) / 10; // 100倍して小数5桁表示用
    int64_t mp_per_100turn = mp_per_turn_x100 * 100; // 100ターン分

    return static_cast<int>(mp_per_100turn);
}

/*!
 * @brief キャラ基本情報及び技能値をメインウィンドウに表示する
 * @param creature クリーチャーへの参照
 * @param xthb 武器等を含めた最終命中率
 * @param damage 打撃修正
 * @param shots 射撃回数
 * @param shot_frac 射撃速度
 * @param display_player_one_line 1行表示用のコールバック関数
 */
static void display_first_page(CreatureEntity &creature, int xthb, int *damage, int shots, int shot_frac)
{
    int xthn = creature.skill_thn + (creature.to_h_m * BTH_PLUS_ADJ);

    int muta_att = 0;
    if (creature.muta.has(PlayerMutationType::HORNS)) {
        muta_att++;
    }
    if (creature.muta.has(PlayerMutationType::SCOR_TAIL)) {
        muta_att++;
    }
    if (creature.muta.has(PlayerMutationType::BEAK)) {
        muta_att++;
    }
    if (creature.muta.has(PlayerMutationType::TRUNK)) {
        muta_att++;
    }
    if (creature.muta.has(PlayerMutationType::TENTACLES)) {
        muta_att++;
    }

    int blows1 = can_attack_with_main_hand(creature) ? creature.num_blow[0] : 0;
    int blows2 = can_attack_with_sub_hand(creature) ? creature.num_blow[1] : 0;
    int xdis = creature.skill_dis;
    int xdev = creature.skill_dev;
    int xsav = creature.skill_sav;
    int xstl = creature.skill_stl;
    int xsrh = creature.skill_srh;
    int xfos = creature.skill_fos;
    int xdig = creature.skill_dig;

    auto sd = likert(xthn, 12);
    display_player_one_line(ENTRY_SKILL_FIGHT, sd.first, sd.second);

    sd = likert(xthb, 12);
    display_player_one_line(ENTRY_SKILL_SHOOT, sd.first, sd.second);

    sd = likert(xsav, 7);
    display_player_one_line(ENTRY_SKILL_SAVING, sd.first, sd.second);

    sd = likert((xstl > 0) ? xstl : -1, 1);
    display_player_one_line(ENTRY_SKILL_STEALTH, sd.first, sd.second);

    sd = likert(xfos, 6);
    display_player_one_line(ENTRY_SKILL_PERCEP, sd.first, sd.second);

    sd = likert(xsrh, 6);
    display_player_one_line(ENTRY_SKILL_SEARCH, sd.first, sd.second);

    sd = likert(xdis, 8);
    display_player_one_line(ENTRY_SKILL_DISARM, sd.first, sd.second);

    sd = likert(xdev, 6);
    display_player_one_line(ENTRY_SKILL_DEVICE, sd.first, sd.second);

    sd = likert(xdig, 4);
    display_player_one_line(ENTRY_SKILL_DIG, sd.first, sd.second);

    if (!muta_att) {
        display_player_one_line(ENTRY_BLOWS, format("%d+%d", blows1, blows2), TERM_L_BLUE);
    } else {
        display_player_one_line(ENTRY_BLOWS, format("%d+%d+%d", blows1, blows2, muta_att), TERM_L_BLUE);
    }

    display_player_one_line(ENTRY_SHOTS, format("%d.%02d", shots, shot_frac), TERM_L_BLUE);

    std::string desc;
    if ((damage[0] + damage[1]) == 0) {
        desc = "nil!";
    } else {
        desc = format("%d+%d", blows1 * damage[0] / 100, blows2 * damage[1] / 100);
    }

    display_player_one_line(ENTRY_AVG_DMG, desc, TERM_L_BLUE);
    display_player_one_line(ENTRY_INFRA, format("%d feet", creature.see_infra * 10), TERM_WHITE);

    // HP回復量/100ターンの計算
    int hp_regen_amount = calculate_hp_regen_rate(creature);
    std::string hp_regen_desc = format("%+d.%05d", hp_regen_amount / 100000, hp_regen_amount % 100000);
    display_player_one_line(ENTRY_HP_REGEN, hp_regen_desc, TERM_L_BLUE);

    // MP回復量/100ターンの計算
    int mp_regen_amount = calculate_mp_regen_rate(creature);
    std::string mp_regen_desc = format("%+d.%05d", mp_regen_amount / 100000, mp_regen_amount % 100000);
    display_player_one_line(ENTRY_MP_REGEN, mp_regen_desc, TERM_L_BLUE);
}

/*!
 * @brief プレイヤーステータスの1ページ目各種詳細をまとめて表示する
 * Prints ratings on certain abilities
 * @param creature クリーチャーへの参照
 * @param display_player_one_line 1行表示用のコールバック関数
 * @details
 * This code is "imitated" elsewhere to "dump" a character sheet.
 */
void display_player_various(CreatureEntity &creature)
{
    ItemEntity *o_ptr;
    o_ptr = creature.inventory[INVEN_BOW].get();
    int tmp = creature.to_h_b + o_ptr->to_h;
    int xthb = creature.skill_thb + (tmp * BTH_PLUS_ADJ);
    int shots = 0;
    int shot_frac = 0;
    calc_shot_params(creature, o_ptr, &shots, &shot_frac);

    int damage[2];
    int to_h[2];
    calc_two_hands(creature, damage, to_h);
    display_first_page(creature, xthb, damage, shots, shot_frac);
}
