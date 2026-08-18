#pragma once

#include "system/angband.h"

#include "combat/combat-options-type.h"
#include "effect/attribute-types.h"
#include "object-enchant/tr-flags.h"

class ItemEntity;
class CreatureEntity;
MULTIPLY mult_slaying(CreatureEntity &creature, MULTIPLY mult, const TrFlags &flags, const CreatureEntity &target);
MULTIPLY mult_brand(CreatureEntity &creature, MULTIPLY mult, const TrFlags &flags, const CreatureEntity &target);
int calc_attack_damage_with_slay(CreatureEntity &creature, ItemEntity *o_ptr, int tdam, const CreatureEntity &target, combat_options mode, bool thrown);
int calc_weapon_melee_damage(CreatureEntity &attacker, ItemEntity &weapon, const CreatureEntity &target, int hand);
int apply_weapon_vampiric_drain(CreatureEntity &attacker, const ItemEntity &weapon, const CreatureEntity &target, int weapon_damage);
AttributeFlags melee_attribute(CreatureEntity &creature, ItemEntity *o_ptr, combat_options mode);
