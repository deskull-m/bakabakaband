#pragma once

#include "util/point-2d.h"

class Direction;
class CreatureEntity;
void lite_room(CreatureEntity &creature, const Pos2D &pos_start);
bool starlight(CreatureEntity &creature, bool magic);
void unlite_room(CreatureEntity &creature, const Pos2D &pos_start);
bool lite_area(CreatureEntity &creature, int dam, int rad);
bool unlite_area(CreatureEntity &creature, int dam, int rad);
bool lite_line(CreatureEntity &creature, const Direction &dir, int dam);
