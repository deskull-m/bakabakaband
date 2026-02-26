#pragma once

#include "util/point-2d.h"

enum class VaultTypeId : int16_t {
    NONE = 0
};

class CreatureEntity;
class FloorType;
void vault_monsters(CreatureEntity &creature, const Pos2D &pos_center, int num);
void vault_objects(CreatureEntity &creature, const Pos2D &pos_center, int num);
void vault_traps(FloorType &floor, const Pos2D &pos_center, const Pos2DVec &distribution, int num);
