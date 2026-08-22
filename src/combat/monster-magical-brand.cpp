/*!
 * @brief モンスターが装備した魔術ブランド武器 (TR_BRAND_MAGIC) の追加効果処理
 * @details
 * プレイヤーの select_magical_brand_effect() / change_monster_stat() と同じ確率・効果で、
 * 魔術ブランド武器を装備したモンスターの近接打撃に追加ダイス・朦朧・恐怖・魔力吸収を与える。
 * 調査 (PROBE) はプレイヤーへの情報提示効果でモンスター攻撃者には意味が無いため非適用。
 * プレイヤー標的への魔力吸収 (DISPELL) はプレイヤーバフ解呪の単純プリミティブが無いため非適用。
 */

#include "combat/monster-magical-brand.h"
#include "monster-race/race-ability-mask.h"
#include "monster-race/race-flags-resistance.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "object-enchant/tr-types.h"
#include "player-attack/player-attack.h"
#include "status/bad-status-setter.h"
#include "system/creature-entity.h"
#include "system/creature-timed-effect-types.h"
#include "system/floor/floor-info.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "term/z-rand.h"
#include "util/dice.h"
#include <algorithm>

namespace {
/*!
 * @brief 状態異常の基本強度 (プレイヤーの attack_stun/scare/confuse と同じ算出)
 */
int magical_brand_status_power(CreatureEntity &attacker)
{
    return 10 + randint0(attacker.get_level()) / 5;
}

/*!
 * @brief 魔術ブランドによる朦朧を対象へ与える (プレイヤー・モンスター両対応)
 */
void inflict_magical_stun(CreatureEntity &attacker, CreatureEntity &target, MONSTER_IDX target_m_idx)
{
    const auto power = magical_brand_status_power(attacker);
    if (target.is_player()) {
        BadStatusSetter(target).mod_stun(power);
        return;
    }

    const auto &monrace = target.get_monrace();
    if (monrace.resistance_flags.has(MonsterResistanceType::NO_STUN) || evaluate_percent(monrace.level)) {
        return;
    }

    set_monster_stunned(*attacker.get_floor(), target_m_idx, target.get_remaining_stun() + power);
}

/*!
 * @brief 魔術ブランドによる恐怖を対象へ与える (プレイヤー・モンスター両対応)
 */
void inflict_magical_scare(CreatureEntity &attacker, CreatureEntity &target, MONSTER_IDX target_m_idx)
{
    const auto power = magical_brand_status_power(attacker);
    if (target.is_player()) {
        BadStatusSetter(target).mod_fear(power);
        return;
    }

    const auto &monrace = target.get_monrace();
    if (monrace.resistance_flags.has(MonsterResistanceType::NO_FEAR) || evaluate_percent(monrace.level)) {
        return;
    }

    set_monster_monfear(*attacker.get_floor(), target_m_idx, target.get_remaining_fear() + power);
}

/*!
 * @brief 魔術ブランドによる魔力吸収 (対象の強化解除 + 攻撃側の MP 回復)
 * @details プレイヤーの attack_dispel と同じく、攻撃能力を持つモンスターのみ対象。プレイヤー標的は
 * バフ解呪の単純プリミティブが無いため非適用。
 */
void inflict_magical_dispel(CreatureEntity &attacker, CreatureEntity &target, CreatureEntity &player, MONSTER_IDX target_m_idx)
{
    if (target.is_player()) {
        return;
    }

    const auto &monrace = target.get_monrace();
    if (monrace.ability_flags.has_none_of(RF_ABILITY_ATTACK_MASK) && monrace.ability_flags.has_none_of(RF_ABILITY_INDIRECT_MASK)) {
        return;
    }

    auto dd = 2;
    if (target.get_timed_effect(CreatureTimedEffect::DECELERATION)) {
        dd += 1;
    }
    if (target.get_timed_effect(CreatureTimedEffect::ACCELERATION)) {
        dd += 2;
    }
    if (target.get_timed_effect(CreatureTimedEffect::INVULNERABILITY)) {
        dd += 3;
    }

    dispel_monster_status(player, target_m_idx);
    const auto sp = Dice::roll(dd, 8);
    attacker.set_current_mp(std::min(attacker.get_max_mp(), attacker.get_current_mp() + sp));
}
}

/*!
 * @brief 魔術ブランド (TR_BRAND_MAGIC) の効果を抽選する
 * @return 抽選された魔術効果。TR_BRAND_MAGIC を持たなければ NONE
 * @details 確率はプレイヤーの select_magical_brand_effect() と一致させる。
 */
MagicalBrandEffectType roll_monster_magical_brand_effect(const ItemEntity &weapon)
{
    if (weapon.get_flags().has_not(TR_BRAND_MAGIC)) {
        return MagicalBrandEffectType::NONE;
    }

    if (one_in_(5)) {
        return MagicalBrandEffectType::STUN;
    }

    if (one_in_(5)) {
        return MagicalBrandEffectType::SCARE;
    }

    if (one_in_(10)) {
        return MagicalBrandEffectType::DISPELL;
    }

    if (one_in_(16)) {
        return MagicalBrandEffectType::PROBE;
    }

    return MagicalBrandEffectType::EXTRA;
}

/*!
 * @brief 魔術効果が与える追加ダメージダイス数 (プレイヤーの magical_brand_extra_dice と同じ)
 */
int monster_magical_brand_extra_dice(MagicalBrandEffectType effect)
{
    switch (effect) {
    case MagicalBrandEffectType::NONE:
        return 0;
    case MagicalBrandEffectType::EXTRA:
        return 1;
    default:
        return 2;
    }
}

/*!
 * @brief 魔術ブランドの状態異常効果 (朦朧/恐怖/魔力吸収) を対象へ適用する
 * @details 調査 (PROBE) と追加ダイス (EXTRA) は状態異常を伴わないため何もしない
 * (追加ダイスは calc_weapon_melee_damage 側で反映済み)。
 */
void apply_monster_magical_brand_status(CreatureEntity &attacker, MagicalBrandEffectType effect, CreatureEntity &target,
    CreatureEntity &player, MONSTER_IDX target_m_idx)
{
    switch (effect) {
    case MagicalBrandEffectType::STUN:
        inflict_magical_stun(attacker, target, target_m_idx);
        break;
    case MagicalBrandEffectType::SCARE:
        inflict_magical_scare(attacker, target, target_m_idx);
        break;
    case MagicalBrandEffectType::DISPELL:
        inflict_magical_dispel(attacker, target, player, target_m_idx);
        break;
    default:
        break;
    }
}
