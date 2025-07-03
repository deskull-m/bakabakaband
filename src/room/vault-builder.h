#pragma once

#include "util/point-2d.h"

enum class VaultTypeId : int16_t {
    NONE = 0
};

class FloorType;
class PlayerType;
void vault_monsters(PlayerType *player_ptr, const Pos2D &pos_center, int num);
void vault_objects(PlayerType *player_ptr, const Pos2D &pos_center, int num);
void vault_traps(FloorType &floor, const Pos2D &pos_center, const Pos2DVec &distribution, int num);
