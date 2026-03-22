#pragma once

#include "system/angband.h"

class CreatureEntity;
class PlayerType;
class ObjectZapRodEntity {
public:
    ObjectZapRodEntity(CreatureEntity &creature);
    virtual ~ObjectZapRodEntity() = default;

    void execute(INVENTORY_IDX i_idx);

private:
    PlayerType *player_ptr;

    bool check_can_zap();
};
