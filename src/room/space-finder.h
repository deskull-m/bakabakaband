#pragma once

#include "util/point-2d.h"
#include <tl/optional.hpp>

class CreatureEntity;
class DungeonData;
tl::optional<Pos2D> find_space(CreatureEntity &creature, DungeonData *dd_ptr, int height, int width);
