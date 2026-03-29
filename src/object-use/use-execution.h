#pragma once

#include "system/angband.h"

class CreatureEntity;
class ObjectUseEntity {
public:
    ObjectUseEntity(CreatureEntity &creature, INVENTORY_IDX i_idx);
    virtual ~ObjectUseEntity() = default;

    void execute();

private:
    CreatureEntity *creature_ptr;
    INVENTORY_IDX i_idx;

    bool check_can_use();
};
