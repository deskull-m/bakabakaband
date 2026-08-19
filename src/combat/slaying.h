#pragma once

#include "system/angband.h"

#include "combat/combat-options-type.h"
#include "effect/attribute-types.h"
#include "object-enchant/tr-flags.h"

class ItemEntity;
class CreatureEntity;
MULTIPLY mult_slaying(CreatureEntity &creature, MULTIPLY mult, const TrFlags &flags, CreatureEntity &target);
MULTIPLY mult_brand(CreatureEntity &creature, MULTIPLY mult, const TrFlags &flags, CreatureEntity &target);
int calc_attack_damage_with_slay(CreatureEntity &creature, ItemEntity *o_ptr, int tdam, CreatureEntity &target, combat_options mode, bool thrown);
int calc_weapon_melee_damage(CreatureEntity &attacker, ItemEntity &weapon, CreatureEntity &target, int hand);
int drain_life_to_attacker(CreatureEntity &attacker, const CreatureEntity &target, int amount);
int apply_weapon_vampiric_drain(CreatureEntity &attacker, const ItemEntity &weapon, const CreatureEntity &target, int weapon_damage);
bool does_weapon_cause_earthquake(const ItemEntity &weapon, int weapon_damage);
AttributeFlags melee_attribute(CreatureEntity &creature, ItemEntity *o_ptr, combat_options mode);
