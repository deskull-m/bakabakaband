#pragma once

#include "floor/geometry.h"

class CreatureEntity;
Direction get_aim_dir(CreatureEntity &subject, bool enable_repeat = true);
Direction get_direction(CreatureEntity &creature);
Direction get_rep_dir(CreatureEntity &creature, bool under = false);
