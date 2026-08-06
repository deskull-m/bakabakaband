#pragma once

#include "system/angband.h"

struct ProjectResult;
enum class MonsterAbilityType;
enum class AttributeType;

class CreatureEntity;
bool clean_shot(CreatureEntity &creature, POSITION y1, POSITION x1, POSITION y2, POSITION x2, bool is_friend);
bool summon_possible(CreatureEntity &creature, POSITION y1, POSITION x1);
bool raise_possible(CreatureEntity &creature, const CreatureEntity &target);
bool spell_is_inate(MonsterAbilityType spell);
ProjectResult beam(CreatureEntity &creature, MONSTER_IDX m_idx, POSITION y, POSITION x, AttributeType typ, int dam_hp, int target_type);
ProjectResult bolt(CreatureEntity &creature, MONSTER_IDX m_idx, POSITION y, POSITION x, AttributeType typ, int dam_hp, int target_type);
ProjectResult pointed(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, AttributeType typ, int dam_hp, int target_type);
ProjectResult ball(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, AttributeType typ, int dam_hp, POSITION rad, int target_type);
ProjectResult breath(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, AttributeType typ, int dam_hp, POSITION rad, int target_type);
ProjectResult rocket(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, AttributeType typ, int dam_hp, POSITION rad, int target_type);
