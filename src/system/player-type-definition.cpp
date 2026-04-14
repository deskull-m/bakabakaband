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
    case CreatureTimedEffect::HERO:
        return this->hero;
    case CreatureTimedEffect::BERSERK:
        return this->berserk;
    case CreatureTimedEffect::BLESSED:
        return this->blessed;
    case CreatureTimedEffect::SHIELD:
        return this->shield;
    case CreatureTimedEffect::ULTIMATE_RESISTANCE:
        return this->ult_res;
    case CreatureTimedEffect::WRAITH_FORM:
        return this->wraith_form;
    case CreatureTimedEffect::TIM_ESP:
        return this->tim_esp;
    case CreatureTimedEffect::TIM_STEALTH:
        return this->tim_stealth;
    case CreatureTimedEffect::TIM_REGEN:
        return this->tim_regen;
    case CreatureTimedEffect::TSUYOSHI:
        return this->tsuyoshi;
    case CreatureTimedEffect::TIM_INVIS:
        return this->tim_invis;
    case CreatureTimedEffect::TIM_INFRA:
        return this->tim_infra;
    case CreatureTimedEffect::OPPOSE_ACID:
        return this->oppose_acid;
    case CreatureTimedEffect::OPPOSE_ELEC:
        return this->oppose_elec;
    case CreatureTimedEffect::OPPOSE_FIRE:
        return this->oppose_fire;
    case CreatureTimedEffect::OPPOSE_COLD:
        return this->oppose_cold;
    case CreatureTimedEffect::OPPOSE_POIS:
        return this->oppose_pois;
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
    case CreatureTimedEffect::HERO:
        this->hero = value;
        break;
    case CreatureTimedEffect::BERSERK:
        this->berserk = value;
        break;
    case CreatureTimedEffect::BLESSED:
        this->blessed = value;
        break;
    case CreatureTimedEffect::SHIELD:
        this->shield = value;
        break;
    case CreatureTimedEffect::ULTIMATE_RESISTANCE:
        this->ult_res = value;
        break;
    case CreatureTimedEffect::WRAITH_FORM:
        this->wraith_form = value;
        break;
    case CreatureTimedEffect::TIM_ESP:
        this->tim_esp = value;
        break;
    case CreatureTimedEffect::TIM_STEALTH:
        this->tim_stealth = value;
        break;
    case CreatureTimedEffect::TIM_REGEN:
        this->tim_regen = value;
        break;
    case CreatureTimedEffect::TSUYOSHI:
        this->tsuyoshi = value;
        break;
    case CreatureTimedEffect::TIM_INVIS:
        this->tim_invis = value;
        break;
    case CreatureTimedEffect::TIM_INFRA:
        this->tim_infra = value;
        break;
    case CreatureTimedEffect::OPPOSE_ACID:
        this->oppose_acid = value;
        break;
    case CreatureTimedEffect::OPPOSE_ELEC:
        this->oppose_elec = value;
        break;
    case CreatureTimedEffect::OPPOSE_FIRE:
        this->oppose_fire = value;
        break;
    case CreatureTimedEffect::OPPOSE_COLD:
        this->oppose_cold = value;
        break;
    case CreatureTimedEffect::OPPOSE_POIS:
        this->oppose_pois = value;
        break;
    default:
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
