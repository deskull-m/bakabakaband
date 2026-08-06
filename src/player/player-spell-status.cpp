#include "player/player-spell-status.h"
#include "player-base/player-class.h"
#include "system/creature-entity.h"
#include "util/bit-flags-calculator.h"

PlayerSpellStatus::PlayerSpellStatus(CreatureEntity &creature)
{
    this->creature_ptr = &creature;
}

PlayerSpellStatus::Realm PlayerSpellStatus::realm1() const
{
    return Realm(*this->creature_ptr, true);
}

PlayerSpellStatus::Realm PlayerSpellStatus::realm2() const
{
    return Realm(*this->creature_ptr, false);
}

PlayerSpellStatus::Realm::Realm(CreatureEntity &creature, bool is_realm1)
    : is_realm1(is_realm1)
{
    this->creature_ptr = &creature;
}

void PlayerSpellStatus::Realm::initialize()
{
    const auto realm_idx = this->is_realm1 ? 0 : 1;
    const auto is_sorcerer = CreatureClass(*this->creature_ptr).equals(PlayerClassType::SORCERER);
    const BIT_FLAGS initial = is_sorcerer ? 0xffffffffU : 0;
    this->creature_ptr->set_spell_learned_flags(realm_idx, initial);
    this->creature_ptr->set_spell_worked_flags(realm_idx, initial);
    this->creature_ptr->set_spell_forgotten_flags(realm_idx, 0);

    auto is_erase_spell_id = this->is_realm1 ? [](int spell_id) { return spell_id >= 32; } : [](int spell_id) { return spell_id < 32; };
    std::erase_if(this->creature_ptr->spell_order_learned, is_erase_spell_id);
}

bool PlayerSpellStatus::Realm::is_nothing_learned() const
{
    const auto realm_idx = this->is_realm1 ? 0 : 1;
    return this->creature_ptr->get_spell_learned_flags(realm_idx) == 0;
}

bool PlayerSpellStatus::Realm::is_learned(int spell_id) const
{
    const auto realm_idx = this->is_realm1 ? 0 : 1;
    return this->creature_ptr->has_learned_spell(realm_idx, spell_id);
}

bool PlayerSpellStatus::Realm::is_worked(int spell_id) const
{
    const auto realm_idx = this->is_realm1 ? 0 : 1;
    return this->creature_ptr->has_worked_spell(realm_idx, spell_id);
}

bool PlayerSpellStatus::Realm::is_forgotten(int spell_id) const
{
    const auto realm_idx = this->is_realm1 ? 0 : 1;
    return this->creature_ptr->has_forgotten_spell(realm_idx, spell_id);
}

void PlayerSpellStatus::Realm::set_learned(int spell_id, bool value)
{
    const auto realm_idx = this->is_realm1 ? 0 : 1;
    this->creature_ptr->set_learned_spell(realm_idx, spell_id, value);
}

void PlayerSpellStatus::Realm::set_worked(int spell_id, bool value)
{
    const auto realm_idx = this->is_realm1 ? 0 : 1;
    this->creature_ptr->set_worked_spell(realm_idx, spell_id, value);
}

void PlayerSpellStatus::Realm::set_forgotten(int spell_id, bool value)
{
    const auto realm_idx = this->is_realm1 ? 0 : 1;
    this->creature_ptr->set_forgotten_spell(realm_idx, spell_id, value);
}
