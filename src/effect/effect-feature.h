#pragma once

#include "effect/attribute-types.h"
#include "system/angband.h"

class CreatureEntity;
bool affect_feature(CreatureEntity &creature, MONSTER_IDX src_idx, POSITION r, POSITION y, POSITION x, int dam, AttributeType typ);
