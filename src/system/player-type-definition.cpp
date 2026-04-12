#include "system/player-type-definition.h"
#include "inventory/inventory-slot-types.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"
#include "timed-effect/timed-effects.h"
#include <range/v3/algorithm.hpp>

/*!
 * @brief プレイヤー構造体実体 / Static player info record
 */
PlayerType p_body;

/*!
 * @brief プレイヤー構造体へのグローバル参照ポインタ / Pointer to the player info
 */
PlayerType *p_ptr = &p_body;

PlayerType::PlayerType()
{
    this->inventory.resize(INVEN_TOTAL);
    ranges::generate(this->inventory, [] { return std::make_shared<ItemEntity>(); });
    this->timed_effects = std::make_shared<TimedEffects>();
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
    const auto &eff = *this->effects();
    switch (effect) {
    case CreatureTimedEffect::STUN:
        return eff.stun().current();
    case CreatureTimedEffect::CONFUSION:
        return eff.confusion().current();
    case CreatureTimedEffect::FEAR:
        return eff.fear().current();
    case CreatureTimedEffect::INVULNERABILITY:
        return this->invuln;
    case CreatureTimedEffect::ACCELERATION:
        return eff.acceleration().current();
    case CreatureTimedEffect::DECELERATION:
        return eff.deceleration().current();
    case CreatureTimedEffect::SLEEP_OR_PARALYSIS:
        return eff.paralysis().current();
    case CreatureTimedEffect::BLINDNESS:
        return eff.blindness().current();
    case CreatureTimedEffect::PARALYSIS:
        return eff.paralysis().current();
    default:
        return 0;
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
    case CreatureTimedEffect::INVULNERABILITY:
        this->invuln = value;
        break;
    case CreatureTimedEffect::ACCELERATION:
        eff.acceleration().set(value);
        break;
    case CreatureTimedEffect::DECELERATION:
        eff.deceleration().set(value);
        break;
    case CreatureTimedEffect::SLEEP_OR_PARALYSIS:
        eff.paralysis().set(value);
        break;
    case CreatureTimedEffect::BLINDNESS:
        eff.blindness().set(value);
        break;
    case CreatureTimedEffect::PARALYSIS:
        eff.paralysis().set(value);
        break;
    default:
        break;
    }
}
