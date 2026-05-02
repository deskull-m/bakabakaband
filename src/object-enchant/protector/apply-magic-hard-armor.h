#pragma once

#include "object-enchant/protector/apply-magic-armor.h"
#include "system/angband.h"

class CreatureEntity;
class ItemEntity;
class HardArmorEnchanter : public ArmorEnchanter {
public:
    HardArmorEnchanter(CreatureEntity &creature, ItemEntity *o_ptr, DEPTH level, int power);
    HardArmorEnchanter(const HardArmorEnchanter &) = delete;
    HardArmorEnchanter(HardArmorEnchanter &&) = delete;
    HardArmorEnchanter &operator=(const HardArmorEnchanter &) = delete;
    HardArmorEnchanter &operator=(HardArmorEnchanter &&) = delete;
    void apply_magic() override;

protected:
    void sval_enchant() override{};
    void give_high_ego_index() override{};
};
