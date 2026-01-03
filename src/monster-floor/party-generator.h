#pragma once

#include "util/point-2d.h"

class PlayerType;

bool alloc_creature_party(PlayerType *player_ptr, const Pos2D &pos_center);
