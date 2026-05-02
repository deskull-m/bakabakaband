#pragma once

#include "system/angband.h"

class CreatureEntity;
class ItemEntity;
class ObjectReadEntity {
public:
    ObjectReadEntity(CreatureEntity &creature, INVENTORY_IDX i_idx);
    ObjectReadEntity &operator=(const ObjectReadEntity &) = delete;
    ObjectReadEntity &operator=(ObjectReadEntity &&) = delete;
    virtual ~ObjectReadEntity() = default;

    void execute(bool known);

private:
    CreatureEntity &creature;
    INVENTORY_IDX i_idx;

    bool can_read() const;
    void change_virtue_as_read(ItemEntity &o_ref);
    void gain_exp_from_item_use(ItemEntity *o_ptr, bool is_identified);
};
