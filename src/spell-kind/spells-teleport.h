#pragma once

#include "spell/spells-util.h"
#include "system/angband.h"
#include "util/point-2d.h"

class Direction;
class CreatureEntity;
bool teleport_monster(CreatureEntity &creature, const Direction &dir, int distance);
bool teleport_swap(CreatureEntity &creature, const Direction &dir);
bool teleport_away(CreatureEntity &creature, MONSTER_IDX m_idx, POSITION dis, teleport_flags mode);
void teleport_monster_to(CreatureEntity &creature, MONSTER_IDX m_idx, POSITION ty, POSITION tx, int power, teleport_flags mode);
bool teleport_player_aux(CreatureEntity &creature, POSITION dis, bool is_quantum_effect, teleport_flags mode);
bool teleport_player(CreatureEntity &creature, POSITION dis, BIT_FLAGS mode);
bool teleport_creature(CreatureEntity &target, POSITION dis, teleport_flags mode);
void teleport_creature_to(CreatureEntity &target, const Pos2D &pos, teleport_flags mode);
void teleport_player_away(MONSTER_IDX m_idx, CreatureEntity &creature, POSITION dis, bool is_quantum_effect);
void teleport_player_to(CreatureEntity &creature, POSITION ny, POSITION nx, teleport_flags mode);
void teleport_away_followable(CreatureEntity &creature, MONSTER_IDX m_idx);
bool dimension_door(CreatureEntity &creature);
bool exe_dimension_door(CreatureEntity &creature, const Pos2D &pos);
