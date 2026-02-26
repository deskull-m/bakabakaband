#pragma once

#include "system/angband.h"
#include "util/point-2d.h"

class CreatureEntity;
class DungeonData;
class FloorType;
void add_river(FloorType &floor, DungeonData *dd_ptr);
void build_streamer(CreatureEntity &creature, FEAT_IDX feat, int chance);
void place_trees(CreatureEntity &creature, const Pos2D &pos);
void destroy_level(CreatureEntity &creature);
