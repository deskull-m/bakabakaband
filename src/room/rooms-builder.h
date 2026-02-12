#pragma once

#include "system/angband.h"

class CreatureEntity;
void build_lake(CreatureEntity &creature, int type);
void build_cavern(CreatureEntity &creature);
void build_small_room(CreatureEntity &creature, POSITION x0, POSITION y0);
void add_outer_wall(CreatureEntity &creature, POSITION x, POSITION y, int light, POSITION x1, POSITION y1, POSITION x2, POSITION y2);
POSITION dist2(POSITION x1, POSITION y1, POSITION x2, POSITION y2, POSITION h1, POSITION h2, POSITION h3, POSITION h4);
void build_recursive_room(CreatureEntity &creature, POSITION x1, POSITION y1, POSITION x2, POSITION y2, int power);
void build_room(CreatureEntity &creature, POSITION x1, POSITION x2, POSITION y1, POSITION y2);
