#pragma once

#include "effect/attribute-types.h"
#include "system/angband.h"

enum class FixedArtifactId : short;
class CreatureEntity;
class MonsterDeath;
void monster_death(CreatureEntity &creature, MONSTER_IDX m_idx, bool drop_item, AttributeFlags attribute_flags, bool killed_by_player);
void monster_death(CreatureEntity &creature, MONSTER_IDX m_idx, bool drop_item, AttributeType type, bool killed_by_player);
bool drop_single_artifact(CreatureEntity &creature, MonsterDeath *md_ptr, FixedArtifactId a_idx);
