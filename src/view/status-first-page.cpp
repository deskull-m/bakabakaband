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
#include "monster-race/race-ability-flags.h"
#include "monster-race/race-behavior-flags.h"
#include "monster-race/race-feature-flags.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-misc-flags.h"
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
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/terrain/terrain-definition.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
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
    *shots = creature.get_num_fire() * 100;
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

    if (creature.get_level() >= 10) {
        (*shots)++;
    }
    if (creature.get_level() >= 30) {
        (*shots)++;
    }
    if (creature.get_level() >= 45) {
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
    PLAYER_LEVEL level = creature.get_level();
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
    bool impact = creature.has_impact_flag() != 0;
    WEIGHT weight = creature.get_level() * calc_monk_attack_weight(creature);
    int to_h = creature.get_level() * 7 / 10; // 命中計算が煩雑なのでおよその値を使用する

    *basedam = calc_expect_crit(creature, weight, to_h, *basedam, creature.get_to_h(0), false, impact, 100);

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
        damage[i] = creature.get_dis_to_d(i) * 100;
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
        const auto mindice = (o_ptr->damage_dice.num + creature.damage_dice_bonus[i].num);
        const auto maxdice = mindice * (o_ptr->damage_dice.sides + creature.damage_dice_bonus[i].sides);

        basedam = calc_expect_dice(creature, mindice, creature.get_to_h(i), o_ptr);
        basedam += calc_expect_dice(creature, maxdice, creature.get_to_h(i), o_ptr);
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
    // プレイヤー基準の統一ヘルパ。モンスターでも同じ計算式が使える。
    const int regen_amount = compute_regen_amount(creature);

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
    // プレイヤー基準の統一ヘルパ。モンスターでも同じ計算式が使える。
    const int regen_amount = compute_regen_amount(creature);

    // ペットの維持コスト (モンスターでも calculate_upkeep は flore m_list を巡回するが
    // 通常モンスターはペットを持たないため 0 となる)
    int upkeep_factor = calculate_upkeep(creature);
    if (creature.is_player()) {
        CreatureClass pc(creature);
        if ((creature.get_action() == ACTION_LEARN) || (creature.get_action() == ACTION_HAYAGAKE) || pc.samurai_stance_is(SamuraiStanceType::KOUKIJIN)) {
            upkeep_factor += 100;
        }
    }

    // 回復率を計算 (100分率)
    int32_t regen_rate = regen_amount * 100 - upkeep_factor * PY_REGEN_NORMAL;

    // マイナスの場合は回復しない（減少する）
    if (regen_rate < 0) {
        // 減少量を計算 (表示上はマイナス表示)
        int64_t mp_decay_per_10turn = ((int64_t)creature.get_msp() * (-regen_rate) / 100 + PY_REGEN_MNBASE) >> 16;
        int64_t mp_per_turn_x100 = (mp_decay_per_10turn * 100) / 10;
        int64_t mp_per_100turn = -(mp_per_turn_x100 * 100); // 100ターン分（マイナス）
        return static_cast<int>(mp_per_100turn);
    }

    // 実際の回復量を計算
    int64_t mp_per_10turn = ((int64_t)creature.get_msp() * regen_rate / 100 + PY_REGEN_MNBASE) >> 16;
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
    int xthn = creature.get_skill_to_hit_melee() + (creature.get_to_h_m() * BTH_PLUS_ADJ);

    int muta_att = 0;
    if (creature.get_mutations().has(PlayerMutationType::HORNS)) {
        muta_att++;
    }
    if (creature.get_mutations().has(PlayerMutationType::SCOR_TAIL)) {
        muta_att++;
    }
    if (creature.get_mutations().has(PlayerMutationType::BEAK)) {
        muta_att++;
    }
    if (creature.get_mutations().has(PlayerMutationType::TRUNK)) {
        muta_att++;
    }
    if (creature.get_mutations().has(PlayerMutationType::TENTACLES)) {
        muta_att++;
    }

    int blows1 = can_attack_with_main_hand(creature) ? creature.get_num_blow(0) : 0;
    int blows2 = can_attack_with_sub_hand(creature) ? creature.get_num_blow(1) : 0;
    int xdis = creature.get_skill_disarm();
    int xdev = creature.get_skill_device();
    int xsav = creature.get_skill_save();
    int xstl = creature.get_skill_stealth();
    int xsrh = creature.get_skill_search();
    int xfos = creature.get_skill_perception();
    int xdig = creature.get_skill_dig();

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
    display_player_one_line(ENTRY_INFRA, format("%d feet", creature.get_infravision() * 10), TERM_WHITE);

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
 * @brief モンスターの行動技能値を種族定義から推定計算する
 * @param monrace モンスター種族定義
 * @param skills 出力: スキル値の構造体
 * @details
 * モンスター種族には専用の skill_* フィールドが存在しないため、
 * 種族レベル・感知範囲(aaf)・睡眠値(sleep)・各種フラグ
 * (INVISIBLE / STALKER / COLD_BLOOD / EMPTY_MIND / SMART / STUPID
 * / NO_FEAR / NO_CONF / NO_SLEEP / NO_STUN / RESIST_ALL
 * / KILL_WALL / PASS_WALL / SHOOT 等) と速度から推定値を導出して
 * プレイヤーと同じ尺度で表示する。
 */
struct MonsterActionSkills {
    int skill_thn{}; //!< 打撃命中
    int skill_thb{}; //!< 射撃命中
    int skill_sav{}; //!< 魔法防御
    int skill_stl{}; //!< 隠密
    int skill_srh{}; //!< 探索
    int skill_fos{}; //!< 知覚
    int skill_dis{}; //!< 解除
    int skill_dev{}; //!< 魔道具使用
    int skill_dig{}; //!< 掘削
};

static MonsterActionSkills calc_monster_action_skills(const MonraceDefinition &monrace)
{
    MonsterActionSkills skills{};

    const auto has_smart = monrace.behavior_flags.has(MonsterBehaviorType::SMART);
    const auto has_stupid = monrace.behavior_flags.has(MonsterBehaviorType::STUPID);
    const auto level = static_cast<int>(monrace.level);
    const int speed_diff = static_cast<int>(monrace.speed) - 110;

    // 打撃命中: レベルを基軸に知能で補正
    int thn = level * 4;
    if (has_smart) {
        thn += 10;
    }
    if (has_stupid) {
        thn -= 10;
    }
    skills.skill_thn = std::max(0, thn);

    // 射撃命中: SHOOT 能力を持つ種族のみレベルに比例
    int thb = 0;
    if (monrace.ability_flags.has(MonsterAbilityType::SHOOT)) {
        thb = level * 4;
        if (has_smart) {
            thb += 10;
        }
        if (has_stupid) {
            thb -= 10;
        }
    }
    skills.skill_thb = std::max(0, thb);

    // 魔法防御: レベル + 状態異常無効化フラグの数
    int sav = level;
    if (monrace.resistance_flags.has(MonsterResistanceType::NO_FEAR)) {
        sav += 10;
    }
    if (monrace.resistance_flags.has(MonsterResistanceType::NO_CONF)) {
        sav += 10;
    }
    if (monrace.resistance_flags.has(MonsterResistanceType::NO_SLEEP)) {
        sav += 10;
    }
    if (monrace.resistance_flags.has(MonsterResistanceType::NO_STUN)) {
        sav += 10;
    }
    if (monrace.resistance_flags.has(MonsterResistanceType::RESIST_ALL)) {
        sav += 50;
    }
    skills.skill_sav = std::max(0, sav);

    // 隠密: レベルとステルス系フラグ・速度から推定
    int stl = level / 4;
    if (monrace.misc_flags.has(MonsterMiscType::INVISIBLE)) {
        stl += 10;
    }
    if (monrace.misc_flags.has(MonsterMiscType::STALKER)) {
        stl += 5;
    }
    if (monrace.misc_flags.has(MonsterMiscType::COLD_BLOOD)) {
        stl += 3;
    }
    if (monrace.misc_flags.has(MonsterMiscType::EMPTY_MIND)) {
        stl += 2;
    }
    if (monrace.behavior_flags.has(MonsterBehaviorType::TIMID)) {
        stl += 3;
    }
    if (has_stupid) {
        stl -= 3;
    }
    if (speed_diff > 0) {
        stl += speed_diff / 5;
    }
    skills.skill_stl = std::max(0, stl);

    // 知覚 (skill_fos): 感知範囲 aaf を主軸に、知能と覚醒度で補正
    int fos = static_cast<int>(monrace.aaf) * 2;
    if (has_smart) {
        fos += 20;
    }
    if (has_stupid) {
        fos -= 10;
    }
    fos -= static_cast<int>(monrace.sleep) / 4;
    skills.skill_fos = std::max(0, fos);

    // 探索 (skill_srh): 知覚と同じ系統だが感度は控えめ
    int srh = static_cast<int>(monrace.aaf);
    if (has_smart) {
        srh += 10;
    }
    if (has_stupid) {
        srh -= 5;
    }
    srh -= static_cast<int>(monrace.sleep) / 8;
    skills.skill_srh = std::max(0, srh);

    // 解除: レベル + 知能補正
    int dis = level;
    if (has_smart) {
        dis += 20;
    }
    if (has_stupid) {
        dis -= 10;
    }
    skills.skill_dis = std::max(0, dis);

    // 魔道具使用: レベル + 魔法/特殊能力の保有数 + 知能補正
    int dev = level;
    const auto ability_count = static_cast<int>(monrace.ability_flags.count());
    dev += ability_count * 2;
    if (has_smart) {
        dev += 20;
    }
    if (has_stupid) {
        dev -= 10;
    }
    skills.skill_dev = std::max(0, dev);

    // 掘削: レベル + 壁関連フラグ
    int dig = level * 2;
    if (monrace.feature_flags.has(MonsterFeatureType::KILL_WALL)) {
        dig += 100;
    }
    if (monrace.feature_flags.has(MonsterFeatureType::PASS_WALL)) {
        dig += 50;
    }
    skills.skill_dig = std::max(0, dig);

    return skills;
}

/*!
 * @brief モンスターの行動技能値をステータス画面に表示する
 * @param creature 表示対象のクリーチャー (モンスター)
 * @details
 * プレイヤーと同じ likert 尺度で表示する。モンスター種族定義から都度計算するため、
 * モンスターの skill_* フィールドは使用しない。
 */
static void display_monster_first_page(CreatureEntity &creature)
{
    const auto &monrace = MonraceList::get_instance().get_monrace(creature.get_r_idx());
    const auto skills = calc_monster_action_skills(monrace);

    auto sd = likert(skills.skill_thn, 12);
    display_player_one_line(ENTRY_SKILL_FIGHT, sd.first, sd.second);

    sd = likert(skills.skill_thb, 12);
    display_player_one_line(ENTRY_SKILL_SHOOT, sd.first, sd.second);

    sd = likert(skills.skill_sav, 7);
    display_player_one_line(ENTRY_SKILL_SAVING, sd.first, sd.second);

    sd = likert((skills.skill_stl > 0) ? skills.skill_stl : -1, 1);
    display_player_one_line(ENTRY_SKILL_STEALTH, sd.first, sd.second);

    sd = likert(skills.skill_fos, 6);
    display_player_one_line(ENTRY_SKILL_PERCEP, sd.first, sd.second);

    sd = likert(skills.skill_srh, 6);
    display_player_one_line(ENTRY_SKILL_SEARCH, sd.first, sd.second);

    sd = likert(skills.skill_dis, 8);
    display_player_one_line(ENTRY_SKILL_DISARM, sd.first, sd.second);

    sd = likert(skills.skill_dev, 6);
    display_player_one_line(ENTRY_SKILL_DEVICE, sd.first, sd.second);

    sd = likert(skills.skill_dig, 4);
    display_player_one_line(ENTRY_SKILL_DIG, sd.first, sd.second);

    // HP/MP 回復量はプレイヤーと共通の calculate_*_regen_rate() 経由で算出 (compute_regen_amount を共有)。
    int hp_regen_amount = calculate_hp_regen_rate(creature);
    std::string hp_regen_desc = format("%+d.%05d", hp_regen_amount / 100000, hp_regen_amount % 100000);
    display_player_one_line(ENTRY_HP_REGEN, hp_regen_desc, TERM_L_BLUE);

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
    // 弓・武器スロットを前提とした戦闘能力表示はプレイヤー固有
    if (!creature.is_player()) {
        display_monster_first_page(creature);
        return;
    }

    ItemEntity *o_ptr;
    o_ptr = creature.inventory[INVEN_BOW].get();
    int tmp = creature.get_to_h_b() + o_ptr->to_h;
    int xthb = creature.get_skill_to_hit_bow() + (tmp * BTH_PLUS_ADJ);
    int shots = 0;
    int shot_frac = 0;
    calc_shot_params(creature, o_ptr, &shots, &shot_frac);

    int damage[2];
    int to_h[2];
    calc_two_hands(creature, damage, to_h);
    display_first_page(creature, xthb, damage, shots, shot_frac);
}
