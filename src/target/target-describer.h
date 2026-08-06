#pragma once

#include "system/angband.h"

extern bool show_gold_on_floor;

enum target_type : uint32_t;
class CreatureEntity;
char examine_grid(CreatureEntity &creature, const POSITION y, const POSITION x, target_type mode, concptr info);
