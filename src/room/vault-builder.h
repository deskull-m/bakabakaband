#pragma once

#include "system/angband.h"

enum class VaultTypeId : int16_t {
    NONE = 0
};

class FloorType;
class PlayerType;
void vault_monsters(PlayerType *player_ptr, POSITION y1, POSITION x1, int num);
void vault_objects(PlayerType *player_ptr, POSITION y, POSITION x, int num);
void vault_traps(FloorType &floor, POSITION y, POSITION x, POSITION yd, POSITION xd, int num);
