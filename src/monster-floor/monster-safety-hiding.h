#pragma once

#include "util/point-2d.h"
#include <tl/optional.hpp>

class CreatureEntity;
tl::optional<Pos2D> find_safety(CreatureEntity &creature, short m_idx);
tl::optional<Pos2D> find_hiding(CreatureEntity &creature, short m_idx);
