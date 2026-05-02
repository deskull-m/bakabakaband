#pragma once

#include "object-enchant/protector/abstract-protector-enchanter.h"
#include "system/angband.h"

class CreatureEntity;
class ItemEntity;
class ArmorEnchanter : public AbstractProtectorEnchanter {
public:
    ArmorEnchanter(const ArmorEnchanter &) = delete;
    ArmorEnchanter(ArmorEnchanter &&) = delete;
    ArmorEnchanter &operator=(const ArmorEnchanter &) = delete;
    ArmorEnchanter &operator=(ArmorEnchanter &&) = delete;
    virtual ~ArmorEnchanter() = default;

protected:
    ArmorEnchanter(CreatureEntity &creature, ItemEntity *o_ptr, DEPTH level, int power);

    CreatureEntity &creature;

    void give_ego_index() override;
    void give_cursed() override;
};
