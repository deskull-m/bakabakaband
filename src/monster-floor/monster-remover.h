#pragma once

#include "system/creature-entity.h"
#include "util/point-2d.h"

void delete_monster_idx(CreatureEntity &creature, short m_idx);
void wipe_monsters_list(CreatureEntity &creature);
void delete_monster(CreatureEntity &creature, const Pos2D &pos);
