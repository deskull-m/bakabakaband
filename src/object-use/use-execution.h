#pragma once

#include "system/angband.h"

class CreatureEntity;
class PlayerType;
class ObjectUseEntity {
public:
    ObjectUseEntity(CreatureEntity &creature, INVENTORY_IDX i_idx);
    virtual ~ObjectUseEntity() = default;

    void execute();

private:
    PlayerType *player_ptr;
    INVENTORY_IDX i_idx;

    bool check_can_use();
};
