#include "system/player-type-definition.h"
#include "inventory/inventory-slot-types.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"
#include "timed-effect/timed-effects.h"
#include <range/v3/algorithm.hpp>

/*!
 * @brief プレイヤー構造体実体 / Static player info record
 * @note get_instance() を通してのみアクセスすること。直接参照しないこと。
 */
static PlayerType p_body;

PlayerType::PlayerType()
{
    this->inventory.resize(INVEN_TOTAL);
    ranges::generate(this->inventory, [] { return std::make_shared<ItemEntity>(); });
    this->timed_effects = std::make_shared<TimedEffects>();
}

PlayerType &PlayerType::get_instance()
{
    return p_body;
}

PlayerType *PlayerType::get_instance_ptr()
{
    return &p_body;
}

bool PlayerType::is_valid() const
{
    return true; // プレイヤーは常に有効
}

bool PlayerType::is_dead() const
{
    return this->is_dead_;
}

bool PlayerType::is_player() const
{
    return true;
}

short PlayerType::get_timed_effect(CreatureTimedEffect effect) const
{
    // 一部の時限効果はより高機能な TimedEffects オブジェクト経由で管理している
    const auto &eff = *this->effects();
    switch (effect) {
    case CreatureTimedEffect::STUN:
        return eff.stun().current();
    case CreatureTimedEffect::CONFUSION:
        return eff.confusion().current();
    case CreatureTimedEffect::FEAR:
        return eff.fear().current();
    case CreatureTimedEffect::ACCELERATION:
        return eff.acceleration().current();
    case CreatureTimedEffect::DECELERATION:
        return eff.deceleration().current();
    case CreatureTimedEffect::SLEEP_OR_PARALYSIS:
    case CreatureTimedEffect::PARALYSIS:
        return eff.paralysis().current();
    case CreatureTimedEffect::BLINDNESS:
        return eff.blindness().current();
    default: {
        const auto it = this->timed_effects_map.find(effect);
        return (it != this->timed_effects_map.end()) ? it->second : 0;
    }
    }
}

void PlayerType::set_timed_effect(CreatureTimedEffect effect, short value)
{
    auto &eff = *this->effects();
    switch (effect) {
    case CreatureTimedEffect::STUN:
        eff.stun().set(value);
        break;
    case CreatureTimedEffect::CONFUSION:
        eff.confusion().set(value);
        break;
    case CreatureTimedEffect::FEAR:
        eff.fear().set(value);
        break;
    case CreatureTimedEffect::ACCELERATION:
        eff.acceleration().set(value);
        break;
    case CreatureTimedEffect::DECELERATION:
        eff.deceleration().set(value);
        break;
    case CreatureTimedEffect::SLEEP_OR_PARALYSIS:
    case CreatureTimedEffect::PARALYSIS:
        eff.paralysis().set(value);
        break;
    case CreatureTimedEffect::BLINDNESS:
        eff.blindness().set(value);
        break;
    default:
        this->timed_effects_map[effect] = value;
        break;
    }
}

/*!
 * @brief プレイヤー状態を空の初期値にリセットする
 * @details game-play-initializer 等からのキャラクター再初期化時に使われる。
 *          名前やフロア情報等は呼び出し側が事前退避・事後復元する前提。
 */
void PlayerType::wipe()
{
    *this = {};
}
