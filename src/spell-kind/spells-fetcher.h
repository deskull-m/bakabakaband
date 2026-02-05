#pragma once

#include "system/angband.h"

class CreatureEntity;
class Direction;
void fetch_item(CreatureEntity &creature, const Direction &dir, WEIGHT wgt, bool require_los);
bool fetch_monster(CreatureEntity &creature);
