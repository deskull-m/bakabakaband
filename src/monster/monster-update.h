#pragma once

#include "system/angband.h"

class CreatureEntity;
class MonraceDefinition;
struct turn_flags;
bool update_riding_monster(CreatureEntity &creature, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, POSITION oy, POSITION ox, POSITION ny, POSITION nx);
void update_map_flags(turn_flags *turn_flags_ptr);
void update_lite_flags(turn_flags *turn_flags_ptr, const MonraceDefinition &monrace);
void update_monster_race_flags(CreatureEntity &creature, turn_flags *turn_flags_ptr, const CreatureEntity &target);
void update_monster(CreatureEntity &creature, MONSTER_IDX m_idx, bool full);
void update_monsters(CreatureEntity &creature, bool full);
void update_smart_learn(CreatureEntity &creature, MONSTER_IDX m_idx, int what);
