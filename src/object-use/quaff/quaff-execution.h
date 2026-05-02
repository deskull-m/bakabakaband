#pragma once

#include "system/angband.h"

class CreatureEntity;
class ItemEntity;
class ObjectQuaffEntity {
public:
    ObjectQuaffEntity(CreatureEntity &creature);
    ObjectQuaffEntity &operator=(const ObjectQuaffEntity &) = delete;
    ObjectQuaffEntity &operator=(ObjectQuaffEntity &&) = delete;
    virtual ~ObjectQuaffEntity() = default;

    void execute(INVENTORY_IDX i_idx, bool is_rectal = false);

private:
    CreatureEntity &creature;

    bool can_influence();
    bool can_quaff();
    ItemEntity copy_object(const INVENTORY_IDX i_idx);
    void moisten(const ItemEntity &o_ref);
    void change_virtue_as_quaff(const ItemEntity &o_ref);
};
