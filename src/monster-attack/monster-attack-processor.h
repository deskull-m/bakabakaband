#pragma once

#include "system/angband.h"
#include "util/point-2d.h"

class CreatureEntity;
class Grid;
struct turn_flags;
void exe_monster_attack_to_player(CreatureEntity &creature, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, const Pos2D &pos);
bool process_monster_attack_to_monster(CreatureEntity &creature, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, const Grid &grid, bool can_cross);
