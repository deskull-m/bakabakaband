#pragma once

#include "util/point-2d.h"

class CreatureEntity;
class Chest {
public:
    Chest(CreatureEntity &creature);
    virtual ~Chest() = default;
    void open(bool scatter, const Pos2D &pos, short item_idx);
    void fire_trap(const Pos2D &pos, short item_idx);

private:
    CreatureEntity *creature_ptr;
};
