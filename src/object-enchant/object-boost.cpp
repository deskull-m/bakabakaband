#include "object-enchant/object-boost.h"
#include "artifact/random-art-effects.h"
#include "object-enchant/tr-types.h"
#include "player-ability/player-ability-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/floor/floor-info.h"
#include "system/item-entity.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"

/*!
 * @brief 上質以上のオブジェクトに与えるための各種ボーナスを正規乱数も加えて算出する。
 * Help determine an "enchantment bonus" for an object.
 * @param max ボーナス値の限度
 * @param level ボーナス値に加味する基準生成階
 * @return 算出されたボーナス値
 * @details
 * To avoid floating point but still provide a smooth distribution of bonuses,\n
 * we simply round the results of ENERGY_DIVISION in such a way as to "average" the\n
 * correct floating point value.\n
 *\n
 * This function has been changed.  It uses "randnor()" to choose values from\n
 * a normal distribution, whose mean moves from zero towards the max as the\n
 * level increases, and whose standard deviation is equal to 1/4 of the max,\n
 * and whose values are forced to lie between zero and the max, inclusive.\n
 *\n
 * Since the "level" rarely passes 100 before Morgoth is dead, it is very\n
 * rare to get the "full" enchantment on an object, even a deep levels.\n
 *\n
 * It is always possible (albeit unlikely) to get the "full" enchantment.\n
 *\n
 * A sample distribution of values from "m_bonus(10, N)" is shown below:\n
 *\n
 *   N       0     1     2     3     4     5     6     7     8     9    10\n
 * ---    ----  ----  ----  ----  ----  ----  ----  ----  ----  ----  ----\n
 *   0   66.37 13.01  9.73  5.47  2.89  1.31  0.72  0.26  0.12  0.09  0.03\n
 *   8   46.85 24.66 12.13  8.13  4.20  2.30  1.05  0.36  0.19  0.08  0.05\n
 *  16   30.12 27.62 18.52 10.52  6.34  3.52  1.95  0.90  0.31  0.15  0.05\n
 *  24   22.44 15.62 30.14 12.92  8.55  5.30  2.39  1.63  0.62  0.28  0.11\n
 *  32   16.23 11.43 23.01 22.31 11.19  7.18  4.46  2.13  1.20  0.45  0.41\n
 *  40   10.76  8.91 12.80 29.51 16.00  9.69  5.90  3.43  1.47  0.88  0.65\n
 *  48    7.28  6.81 10.51 18.27 27.57 11.76  7.85  4.99  2.80  1.22  0.94\n
 *  56    4.41  4.73  8.52 11.96 24.94 19.78 11.06  7.18  3.68  1.96  1.78\n
 *  64    2.81  3.07  5.65  9.17 13.01 31.57 13.70  9.30  6.04  3.04  2.64\n
 *  72    1.87  1.99  3.68  7.15 10.56 20.24 25.78 12.17  7.52  4.42  4.62\n
 *  80    1.02  1.23  2.78  4.75  8.37 12.04 27.61 18.07 10.28  6.52  7.33\n
 *  88    0.70  0.57  1.56  3.12  6.34 10.06 15.76 30.46 12.58  8.47 10.38\n
 *  96    0.27  0.60  1.25  2.28  4.30  7.60 10.77 22.52 22.51 11.37 16.53\n
 * 104    0.22  0.42  0.77  1.36  2.62  5.33  8.93 13.05 29.54 15.23 22.53\n
 * 112    0.15  0.20  0.56  0.87  2.00  3.83  6.86 10.06 17.89 27.31 30.27\n
 * 120    0.03  0.11  0.31  0.46  1.31  2.48  4.60  7.78 11.67 25.53 45.72\n
 * 128    0.02  0.01  0.13  0.33  0.83  1.41  3.24  6.17  9.57 14.22 64.07\n
 */
int m_bonus(int max, DEPTH level)
{
    int bonus, stand, extra, value;

    /* Paranoia -- enforce maximal "level" */
    if (level > MAX_DEPTH - 1) {
        level = MAX_DEPTH - 1;
    }

    /* The "bonus" moves towards the max */
    bonus = ((max * level) / MAX_DEPTH);

    /* Hack -- determine fraction of error */
    extra = ((max * level) % MAX_DEPTH);

    /* Hack -- simulate floating point computations */
    if (randint0(MAX_DEPTH) < extra) {
        bonus++;
    }

    /* The "stand" is equal to one quarter of the max */
    stand = (max / 4);

    /* Hack -- determine fraction of error */
    extra = (max % 4);

    /* Hack -- simulate floating point computations */
    if (randint0(4) < extra) {
        stand++;
    }

    if (stand <= 0) {
        stand = 1;
    }
    /* Choose an "interesting" value */
    value = randnor(bonus, stand);

    /* Enforce the minimum value */
    if (value < 0) {
        return 0;
    }

    /* Enforce the maximum value */
    if (value > max) {
        return max;
    }
    return value;
}

/*!
 * @brief 対象のオブジェクトにランダムな能力維持を一つ付加する。/ Choose one random sustain
 * @details 重複の抑止はない。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 */
void one_sustain(ItemEntity *o_ptr)
{
    constexpr static auto sustains = {
        TR_SUST_STR,
        TR_SUST_INT,
        TR_SUST_WIS,
        TR_SUST_DEX,
        TR_SUST_CON,
        TR_SUST_CHR,
    };

    o_ptr->art_flags.set(rand_choice(sustains));
}

/*!
 * @brief オブジェクトにランダムな強いESPを与える
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @return TR_ESP_NONLIVINGがついたならばTRUE
 */
bool add_esp_strong(ItemEntity *o_ptr)
{
    constexpr static auto candidates = {
        TR_ESP_EVIL,
        TR_TELEPATHY,
        TR_ESP_NONLIVING,
    };

    const auto flag = rand_choice(candidates);
    o_ptr->art_flags.set(flag);

    return flag == TR_ESP_NONLIVING;
}

/*!
 * @brief オブジェクトにランダムな弱いESPを与える
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param extra TRUEならばESPの最大付与数が増える(TRUE -> 3+1d6 / FALSE -> 1d3)
 */
void add_esp_weak(ItemEntity *o_ptr, bool extra)
{
    int i;
    tr_type weak_esp_list[] = {
        TR_ESP_ANIMAL,
        TR_ESP_UNDEAD,
        TR_ESP_DEMON,
        TR_ESP_ORC,
        TR_ESP_TROLL,
        TR_ESP_GIANT,
        TR_ESP_DRAGON,
        TR_ESP_HUMAN,
        TR_ESP_GOOD,
        TR_ESP_UNIQUE,
        TR_ESP_NASTY,
    };
    const int MAX_ESP_WEAK = sizeof(weak_esp_list) / sizeof(weak_esp_list[0]);
    const int add_count = std::min(MAX_ESP_WEAK, (extra) ? (3 + randint1(randint1(6))) : randint1(3));

    /* Add unduplicated weak esp flags randomly */
    for (i = 0; i < add_count; ++i) {
        int choice = rand_range(i, MAX_ESP_WEAK - 1);

        o_ptr->art_flags.set(weak_esp_list[choice]);
        weak_esp_list[choice] = weak_esp_list[i];
    }
}

/*!
 * @brief 高級なテレパシー群を付ける
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @details
 * テレパシーの冠など。
 * ESPまたは邪ESPは1d3の種族ESPを得る。
 * 無ESPは3+1d6の種族ESPを得る。
 */
void add_high_telepathy(ItemEntity *o_ptr)
{
    if (add_esp_strong(o_ptr)) {
        add_esp_weak(o_ptr, true);
    } else {
        add_esp_weak(o_ptr, false);
    }
}

/*!
 * @brief テレパシー群を付ける
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @details
 * 鋭敏の帽子など。
 * ESP、邪ESP、無ESPまたは1d3の種族ESP。
 */
void add_low_telepathy(ItemEntity *o_ptr)
{
    if (one_in_(2)) {
        add_esp_strong(o_ptr);
    } else {
        add_esp_weak(o_ptr, false);
    }
}

/*!
 * @brief 対象のオブジェクトに元素耐性を一つ付加する。/ Choose one random element resistance
 * @details 候補は火炎、冷気、電撃、酸のいずれかであり、重複の抑止はない。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 */
void one_ele_resistance(ItemEntity *o_ptr)
{
    constexpr static auto resistances = {
        TR_RES_ACID,
        TR_RES_ELEC,
        TR_RES_COLD,
        TR_RES_FIRE,
    };

    o_ptr->art_flags.set(rand_choice(resistances));
}

/*!
 * @brief 対象のオブジェクトにドラゴン装備向け元素耐性を一つ付加する。/ Choose one random element or poison resistance
 * @details 候補は1/7の確率で毒、6/7の確率で火炎、冷気、電撃、酸のいずれか(one_ele_resistance()のコール)であり、重複の抑止はない。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 */
void one_dragon_ele_resistance(ItemEntity *o_ptr)
{
    if (one_in_(7)) {
        o_ptr->art_flags.set(TR_RES_POIS);
    } else {
        one_ele_resistance(o_ptr);
    }
}

/*!
 * @brief 対象のオブジェクトにランダムな上位耐性を一つ付加する。/ Choose one random high resistance
 * @details 重複の抑止はない。候補は毒、閃光、暗黒、破片、盲目、混乱、地獄、因果混乱、カオス、劣化、恐怖、時間逆転、水、呪力のいずれか。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 */
void one_high_resistance(ItemEntity *o_ptr)
{
    constexpr static auto high_resistances = {
        TR_RES_POIS,
        TR_RES_LITE,
        TR_RES_DARK,
        TR_RES_SHARDS,
        TR_RES_BLIND,
        TR_RES_CONF,
        TR_RES_SOUND,
        TR_RES_NETHER,
        TR_RES_NEXUS,
        TR_RES_CHAOS,
        TR_RES_DISEN,
        TR_RES_FEAR,
        TR_RES_TIME,
        TR_RES_WATER,
        TR_RES_CURSE,
    };

    o_ptr->art_flags.set(rand_choice(high_resistances));
}

/*!
 * @brief ドラゴン装備にランダムな耐性を与える
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 */
void dragon_resist(ItemEntity *o_ptr)
{
    do {
        if (one_in_(4)) {
            one_dragon_ele_resistance(o_ptr);
        } else {
            one_high_resistance(o_ptr);
        }
    } while (one_in_(2));
}

/*!
 * @brief 対象のオブジェクトに耐性を一つ付加する。/ Choose one random resistance
 * @details 1/3で元素耐性(one_ele_resistance())、2/3で上位耐性(one_high_resistance)
 * をコールする。重複の抑止はない。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 */
void one_resistance(ItemEntity *o_ptr)
{
    if (one_in_(3)) {
        one_ele_resistance(o_ptr);
    } else {
        one_high_resistance(o_ptr);
    }
}

/*!
 * @brief 対象のオブジェクトに能力を一つ付加する。/ Choose one random ability
 * @details 候補は浮遊、永久光源+1、透明視、警告、遅消化、急回復、麻痺知らず、経験値維持のいずれか。
 * 重複の抑止はない。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 */
void one_ability(ItemEntity *o_ptr)
{
    if (one_in_(5)) {
        one_low_esp(o_ptr);
        return;
    }

    constexpr static auto abilities = {
        TR_LEVITATION,
        TR_LITE_1,
        TR_SEE_INVIS,
        TR_WARNING,
        TR_SLOW_DIGEST,
        TR_REGEN,
        TR_FREE_ACT,
        TR_HOLD_EXP,
    };

    o_ptr->art_flags.set(rand_choice(abilities));
}

/*!
 * @brief 対象のオブジェクトに弱いESPを一つ付加する。/ Choose one lower rank esp
 * @details 候補は動物、アンデッド、悪魔、オーク、トロル、巨人、
 * ドラゴン、人間、善良、ユニークESPのいずれかであり、重複の抑止はない。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 */
void one_low_esp(ItemEntity *o_ptr)
{
    constexpr static auto low_esps = {
        TR_ESP_ANIMAL,
        TR_ESP_UNDEAD,
        TR_ESP_DEMON,
        TR_ESP_ORC,
        TR_ESP_TROLL,
        TR_ESP_GIANT,
        TR_ESP_DRAGON,
        TR_ESP_HUMAN,
        TR_ESP_GOOD,
        TR_ESP_UNIQUE,
        TR_ESP_NASTY,
    };

    o_ptr->art_flags.set(rand_choice(low_esps));
}

/*!
 * @brief 対象のオブジェクトに発動を一つ付加する。/ Choose one random activation
 * @details 候補多数。ランダムアーティファクトのバイアスには一切依存せず、
 * whileループによる構造で能力的に強力なものほど確率を落としている。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 */
void one_activation(ItemEntity *o_ptr)
{
    RandomArtActType type = RandomArtActType::NONE;
    PERCENTAGE chance = 0;
    while (randint1(100) >= chance) {
        type = randnum1<RandomArtActType>(enum2i(RandomArtActType::MAX) - 1);
        switch (type) {
        case RandomArtActType::SUNLIGHT:
        case RandomArtActType::BO_MISS_1:
        case RandomArtActType::BA_POIS_1:
        case RandomArtActType::BO_ELEC_1:
        case RandomArtActType::BO_ACID_1:
        case RandomArtActType::BO_COLD_1:
        case RandomArtActType::BO_FIRE_1:
        case RandomArtActType::CONFUSE:
        case RandomArtActType::SLEEP:
        case RandomArtActType::QUAKE:
        case RandomArtActType::CURE_LW:
        case RandomArtActType::CURE_MW:
        case RandomArtActType::CURE_POISON:
        case RandomArtActType::BERSERK:
        case RandomArtActType::LIGHT:
        case RandomArtActType::MAP_LIGHT:
        case RandomArtActType::DEST_DOOR:
        case RandomArtActType::STONE_MUD:
        case RandomArtActType::TELEPORT:
            chance = 101;
            break;
        case RandomArtActType::BA_COLD_1:
        case RandomArtActType::BA_FIRE_1:
        case RandomArtActType::HYPODYNAMIA_1:
        case RandomArtActType::TELE_AWAY:
        case RandomArtActType::ESP:
        case RandomArtActType::RESIST_ALL:
        case RandomArtActType::DETECT_ALL:
        case RandomArtActType::RECALL:
        case RandomArtActType::SATIATE:
        case RandomArtActType::RECHARGE:
            chance = 85;
            break;
        case RandomArtActType::TERROR:
        case RandomArtActType::PROT_EVIL:
        case RandomArtActType::ID_PLAIN:
            chance = 75;
            break;
        case RandomArtActType::HYPODYNAMIA_2:
        case RandomArtActType::DRAIN_1:
        case RandomArtActType::BO_MISS_2:
        case RandomArtActType::BA_FIRE_2:
        case RandomArtActType::REST_EXP:
            chance = 66;
            break;
        case RandomArtActType::BA_FIRE_3:
        case RandomArtActType::BA_COLD_3:
        case RandomArtActType::BA_ELEC_3:
        case RandomArtActType::WHIRLWIND:
        case RandomArtActType::DRAIN_2:
        case RandomArtActType::CHARM_ANIMAL:
            chance = 50;
            break;
        case RandomArtActType::SUMMON_ANIMAL:
        case RandomArtActType::ANIM_DEAD:
            chance = 40;
            break;
        case RandomArtActType::DISP_EVIL:
        case RandomArtActType::BA_MISS_3:
        case RandomArtActType::DISP_GOOD:
        case RandomArtActType::BANISH_EVIL:
        case RandomArtActType::GENOCIDE:
        case RandomArtActType::MASS_GENO:
        case RandomArtActType::CHARM_UNDEAD:
        case RandomArtActType::CHARM_OTHER:
        case RandomArtActType::SUMMON_PHANTOM:
        case RandomArtActType::REST_ALL:
        case RandomArtActType::RUNE_EXPLO:
            chance = 33;
            break;
        case RandomArtActType::CALL_CHAOS:
        case RandomArtActType::ROCKET:
        case RandomArtActType::CHARM_ANIMALS:
        case RandomArtActType::CHARM_OTHERS:
        case RandomArtActType::SUMMON_ELEMENTAL:
        case RandomArtActType::CURE_700:
        case RandomArtActType::SPEED:
        case RandomArtActType::MID_SPEED:
        case RandomArtActType::ID_FULL:
        case RandomArtActType::RUNE_PROT:
            chance = 25;
            break;
        case RandomArtActType::CURE_1000:
        case RandomArtActType::XTRA_SPEED:
        case RandomArtActType::HUGE_STINKING_STORM:
        case RandomArtActType::DETECT_XTRA:
        case RandomArtActType::DIM_DOOR:
            chance = 10;
            break;
        case RandomArtActType::TREE_CREATION:
        case RandomArtActType::SUMMON_DEMON:
        case RandomArtActType::SUMMON_UNDEAD:
        case RandomArtActType::WRAITH:
        case RandomArtActType::INVULN:
        case RandomArtActType::ALCHEMY:
            chance = 5;
            break;
        default:
            chance = 0;
        }
    }

    o_ptr->activation_id = type;
    o_ptr->art_flags.set(TR_ACTIVATE);
    o_ptr->timeout = 0;
}

/*!
 * @brief 対象のオブジェクトに王者の指輪向けの上位耐性を一つ付加する。/ Choose one random high resistance
 * @details 候補は閃光、暗黒、破片、盲目、混乱、地獄、因果混乱、カオス、恐怖、時間逆転、水、呪力であり
 * 王者の指輪にあらかじめついている耐性をone_high_resistance()から除外したものである。
 * ランダム付加そのものに重複の抑止はない。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 */
void one_lordly_high_resistance(ItemEntity *o_ptr)
{
    constexpr static auto lordly_high_resistances = {
        TR_RES_LITE,
        TR_RES_DARK,
        TR_RES_SHARDS,
        TR_RES_BLIND,
        TR_RES_CONF,
        TR_RES_SOUND,
        TR_RES_NETHER,
        TR_RES_NEXUS,
        TR_RES_CHAOS,
        TR_RES_FEAR,
        TR_RES_TIME,
        TR_RES_WATER,
        TR_RES_CURSE,
    };

    o_ptr->art_flags.set(rand_choice(lordly_high_resistances));
}

/*!
 * @brief オブジェクトの重量を軽くする
 * @param o_ptr オブジェクト情報への参照ポインタ
 */
void make_weight_ligten(ItemEntity *o_ptr)
{
    o_ptr->weight = (2 * o_ptr->get_baseitem().weight / 3);
}

/*!
 * @brief オブジェクトの重量を重くする
 * @param o_ptr オブジェクト情報への参照ポインタ
 */
void make_weight_heavy(ItemEntity *o_ptr)
{
    o_ptr->weight = (4 * o_ptr->get_baseitem().weight / 3);
}

/*!
 * @brief オブジェクトのベースACを増やす
 * @param o_ptr オブジェクト情報への参照ポインタ
 * @details
 * 1/4を加算。最低+5を保証。
 */
void add_xtra_ac(ItemEntity *o_ptr)
{
    o_ptr->ac += std::max<short>(5, o_ptr->ac / 4);
}
