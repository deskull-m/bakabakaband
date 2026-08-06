#pragma once

#include "system/angband.h"
#include "util/point-2d.h"

class CreatureEntity;
bool earthquake(CreatureEntity &creature, const Pos2D &center, int radius, MONSTER_IDX m_idx = 0);
