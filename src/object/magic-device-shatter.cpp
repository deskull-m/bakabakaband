/*!
 * @file magic-device-shatter.cpp
 * @brief 魔道具 (杖/魔法棒/ロッド) を自ら砕いたときの魔力暴発 (Issue #1770)
 * @details クラシック D&D の「ファイナル・ストライク」を着想にした効果。
 *          `do_cmd_destroy()` で魔道具を壊すと、溜めていた魔力が暴走して
 *          **プレイヤーを中心に魔力 (MANA) のボール**が生じ、術者自身も
 *          その一部を被る。威力は「充填されている残り回数」と
 *          「そのベースアイテムの生成レベル」の双方に依存する。
 */

#include "object/magic-device-shatter.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "object/tval-types.h"
#include "player/player-damage.h"
#include "system/baseitem/baseitem-key.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"
#include "view/display-messages.h"
#include <algorithm>

namespace {
//! 魔力が尽きていても生成レベルぶんだけは暴発する土台 (残り回数 0 回相当の重み)
constexpr auto SHATTER_CHARGELESS_WEIGHT = 5;
//! 威力の除数。`生成レベル * (土台 + 残り回数) / 除数` が魔力ボールの威力になる
constexpr auto SHATTER_DAMAGE_DIVISOR = 5;
//! 魔力ボールの威力上限 (魔法棒を大量に重ねて壊した場合の暴走を抑える)
constexpr auto SHATTER_MAX_DAMAGE = 500;
//! 自傷量の除数。魔力ボールの威力をこれで割った分を術者が被る
constexpr auto SHATTER_SELF_DAMAGE_DIVISOR = 3;
//! 魔力ボールの基本半径
constexpr auto SHATTER_RADIUS_BASE = 2;
//! 半径が 1 増えるのに必要な生成レベル
constexpr auto SHATTER_RADIUS_LEVEL_UNIT = 50;
}

/*!
 * @brief 破壊する魔道具に充填されている残り回数を数える
 * @param item 破壊する魔道具 (破壊前の、スタック全体を表すアイテム)
 * @param amount 破壊する個数
 * @return 残り回数 (魔道具でなければ 0)
 * @details 残り回数の持ち方が 3 種で違う。
 *          - 杖 (`STAFF`): `pval` は **1 本あたり**の回数なので個数倍する。
 *          - 魔法棒 (`WAND`): `pval` は **スタック全体**の回数。`reduce_charges()`
 *            と同じ按分で破壊ぶんを取る。
 *          - ロッド (`ROD`): 回数ではなく `timeout` の充填待ちで管理されるため、
 *            **充填済み (すぐ使える) 本数**を 1 回ぶんと数える。どの個体が充填中かは
 *            ゲーム側も区別しないので、破壊する本数のうち充填済みを優先して
 *            割り当てる (プレイヤーに有利な読み)。
 */
int count_device_remaining_charges(const ItemEntity &item, int amount)
{
    if (amount <= 0) {
        return 0;
    }

    switch (item.bi_key.tval()) {
    case ItemKindType::STAFF:
        return std::max<int>(0, item.pval) * amount;
    case ItemKindType::WAND: {
        const auto number = std::max<int>(1, item.number);
        return std::max<int>(0, item.pval) * std::min(amount, number) / number;
    }
    case ItemKindType::ROD: {
        const auto timeout_per_one = item.get_baseitem_pval();
        const auto number = std::max<int>(1, item.number);
        auto charging = 0;
        if ((timeout_per_one > 0) && (item.timeout > 0)) {
            charging = (item.timeout + timeout_per_one - 1) / timeout_per_one;
        }

        const auto ready = std::max(0, number - std::min(charging, number));
        return std::min(ready, amount);
    }
    default:
        return 0;
    }
}

/*!
 * @brief 魔力暴発で生じる魔力ボールの威力を算出する
 * @param charges 充填されている残り回数
 * @param item_level そのベースアイテムの生成レベル
 * @return 魔力ボールの威力 (最低 1)
 */
int calc_device_shatter_damage(int charges, int item_level)
{
    const auto level = std::max(0, item_level);
    const auto remaining = std::max(0, charges);
    const auto damage = level * (SHATTER_CHARGELESS_WEIGHT + remaining) / SHATTER_DAMAGE_DIVISOR;
    return std::clamp(damage, 1, SHATTER_MAX_DAMAGE);
}

/*!
 * @brief 魔力暴発による術者自身へのダメージを算出する
 * @param ball_damage 魔力ボールの威力
 * @return 自傷量 (最低 1)
 */
int calc_device_shatter_self_damage(int ball_damage)
{
    return std::max(1, ball_damage / SHATTER_SELF_DAMAGE_DIVISOR);
}

/*!
 * @brief 魔力暴発で生じる魔力ボールの半径を算出する
 * @param item_level そのベースアイテムの生成レベル
 * @return 半径
 */
int calc_device_shatter_radius(int item_level)
{
    return SHATTER_RADIUS_BASE + std::max(0, item_level) / SHATTER_RADIUS_LEVEL_UNIT;
}

/*!
 * @brief 魔道具を砕いたときの魔力暴発を起こす
 * @param creature 魔道具を砕いたクリーチャーへの参照
 * @param item 破壊する魔道具 (破壊前の、スタック全体を表すアイテム)
 * @param amount 破壊する個数
 * @return 暴発したならtrue (魔道具でなければfalse)
 * @details 魔力ボールはプレイヤーを中心に `PROJECT_JUMP` で直に生じ、
 *          自傷は別途 `take_hit()` で与える (ボールそのものはプレイヤーを
 *          巻き込まないため)。
 */
bool shatter_magic_device(CreatureEntity &creature, const ItemEntity &item, int amount)
{
    if (!item.bi_key.can_recharge()) {
        return false;
    }

    const auto charges = count_device_remaining_charges(item, amount);
    const auto item_level = item.get_baseitem_level();
    const auto damage = calc_device_shatter_damage(charges, item_level);
    const auto radius = calc_device_shatter_radius(item_level);
    const auto item_name = describe_flavor(creature, item, OD_OMIT_PREFIX);
    msg_print(_("{}に溜められた魔力が暴走した！", "The magic stored in {} runs wild!"), item_name);
    constexpr auto flags = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP;
    (void)project(creature, 0, radius, creature.y, creature.x, damage, AttributeType::MANA, flags);
    const auto self_damage = calc_device_shatter_self_damage(damage);
    (void)take_hit(creature, DAMAGE_NOESCAPE, self_damage, _("魔道具の暴発", "a shattered magic device"));
    return true;
}
