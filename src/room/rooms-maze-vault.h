#pragma once

#include "system/angband.h"
#include "util/point-2d.h"

class CreatureEntity;
void r_visit(CreatureEntity &creature, POSITION y1, POSITION x1, POSITION y2, POSITION x2, int node, DIRECTION dir, int *visited);
void build_maze_vault(CreatureEntity &creature, const Pos2D &center, const Pos2DVec &vec, bool is_vault);
