#pragma once

#include "system/angband.h"

class CreatureEntity;
class ObjectZapWandEntity {
public:
    ObjectZapWandEntity(CreatureEntity &creature);
    ObjectZapWandEntity &operator=(const ObjectZapWandEntity &) = delete;
    ObjectZapWandEntity &operator=(ObjectZapWandEntity &&) = delete;
    virtual ~ObjectZapWandEntity() = default;

    void execute(INVENTORY_IDX i_idx);

private:
    CreatureEntity &creature;

    bool check_can_zap() const;
};
