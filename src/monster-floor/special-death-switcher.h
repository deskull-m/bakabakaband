#pragma once
#include "effect/attribute-types.h"
#include "system/angband.h"

class CreatureEntity;
class MonsterDeath;
void switch_special_death(CreatureEntity &creature, MonsterDeath *md_ptr, AttributeFlags attribute_flags);
