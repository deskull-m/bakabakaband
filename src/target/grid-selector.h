#pragma once

#include "util/point-2d.h"
#include <tl/optional.hpp>

class CreatureEntity;
tl::optional<Pos2D> point_target(CreatureEntity &creature);
