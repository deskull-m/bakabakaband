#pragma once

#include "system/angband.h"

class CreatureEntity;
void wiz_lite(CreatureEntity &creature, bool ninja);
void wiz_dark(CreatureEntity &creature);
void map_area(CreatureEntity &creature, POSITION range);
bool destroy_area(CreatureEntity &creature, const POSITION y1, const POSITION x1, POSITION r, bool in_generate);
