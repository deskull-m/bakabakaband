#pragma once

#include "util/point-2d.h"

class CreatureEntity;
bool pattern_effect(CreatureEntity &creature);
bool pattern_seq(CreatureEntity &creature, const Pos2D &pos);
void pattern_teleport(CreatureEntity &creature);
