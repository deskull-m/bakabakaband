#pragma once

#include "effect/attribute-types.h"
#include "system/angband.h"
#include "util/point-2d.h"

class CreatureEntity;
class FloorType;
bool direct_beam(CreatureEntity &creature, const CreatureEntity &caster, const Pos2D &pos_target);
bool breath_direct(CreatureEntity &creature, const Pos2D &pos_source, const Pos2D &pos_target, int rad, AttributeType typ, bool is_friend);
Pos2D get_project_point(const FloorType &floor, const Pos2D &p_pos, const Pos2D &pos_source, const Pos2D &pos_target_initial, BIT_FLAGS flags);
bool dispel_check_monster(CreatureEntity &creature, MONSTER_IDX m_idx, MONSTER_IDX t_idx);
bool dispel_check(CreatureEntity &creature, MONSTER_IDX m_idx);
