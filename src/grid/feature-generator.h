#pragma once

#include "util/point-2d.h"

class CreatureEntity;
class DungeonDefinition;
class DungeonData;
struct dt_type;
void gen_caverns_and_lakes(CreatureEntity &creature, const DungeonDefinition &dungeon, DungeonData *dd_ptr);
void try_door(CreatureEntity &creature, dt_type *dt_ptr, const Pos2D &pos);
