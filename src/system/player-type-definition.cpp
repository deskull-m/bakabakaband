#include "system/player-type-definition.h"
#include "floor/geometry.h"
#include "inventory/inventory-slot-types.h"
#include "market/arena-entry.h"
#include "monster/monster-util.h"
#include "system/angband-exceptions.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/redrawing-flags-updater.h"
#include "timed-effect/timed-effects.h"
#include "world/world.h"
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

bool PlayerType::is_true_winner() const
{
    const auto &world = AngbandWorld::get_instance();
    const auto &entries = ArenaEntryList::get_instance();
    return (world.total_winner > 0) && (entries.is_player_true_victor());
}

/*!
 * @brief インシデント数加算
 * @param incident_id 加算したいインシデント
 * @param num 加算量
 */
void PlayerType::plus_incident(INCIDENT incidentID, int num)
{
    if (this->incident.count(incidentID) == 0) {
        this->incident[incidentID] = 0;
    }
    this->incident[incidentID] += num;
}

/*!
 * @brief モンスターに乗る
 * @param m_idx 乗るモンスターのID（0で降りる）
 */
void PlayerType::ride_monster(MONSTER_IDX m_idx)
{
    if (is_monster(this->riding)) {
        this->current_floor_ptr->get_monster(this->riding).get_monster_profile().mflag2.reset(MonsterConstantFlagType::RIDING);
    }

    this->riding = m_idx;

    if (is_monster(m_idx)) {
        this->current_floor_ptr->get_monster(m_idx).get_monster_profile().mflag2.set(MonsterConstantFlagType::RIDING);
    }
}

/*!
 * @brief 自身の状態が全快で、かつフロアに影響を与えないかを検証する
 * @return 上記の通りか
 * @todo 時限効果系に分類されるものはいずれTimedEffectsクラスのメソッドとして繰り込みたい
 */
bool PlayerType::is_fully_healthy() const
{
    auto effects = this->effects();
    auto is_fully_healthy = this->hp == this->maxhp;
    is_fully_healthy &= this->csp >= this->msp;
    is_fully_healthy &= !effects->blindness().is_blind();
    is_fully_healthy &= !effects->confusion().is_confused();
    is_fully_healthy &= !effects->poison().is_poisoned();
    is_fully_healthy &= !effects->fear().is_fearful();
    is_fully_healthy &= !effects->stun().is_stunned();
    is_fully_healthy &= !effects->cut().is_cut();
    is_fully_healthy &= !effects->deceleration().is_slow();
    is_fully_healthy &= !effects->paralysis().is_paralyzed();
    is_fully_healthy &= !effects->hallucination().is_hallucinated();
    is_fully_healthy &= !this->word_recall;
    is_fully_healthy &= !this->alter_reality;
    return is_fully_healthy;
}

bool PlayerType::is_located_at_running_destination() const
{
    return (this->y == this->run_py) && (this->x == this->run_px);
}

/*!
 * @brief プレイヤーを指定座標に配置する
 * @param pos 配置先座標
 * @return 配置に成功したらTRUE
 */
bool PlayerType::try_set_position(const Pos2D &pos)
{
    if (this->current_floor_ptr->get_grid(pos).has_monster()) {
        return false;
    }

    this->y = pos.y;
    this->x = pos.x;
    return true;
}

void PlayerType::set_position(const Pos2D &pos)
{
    this->y = pos.y;
    this->x = pos.x;
}

bool PlayerType::in_saved_floor() const
{
    return this->floor_id != 0;
}

bool PlayerType::try_resist_eldritch_horror() const
{
    return evaluate_percent(this->skill_sav) || one_in_(2);
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
