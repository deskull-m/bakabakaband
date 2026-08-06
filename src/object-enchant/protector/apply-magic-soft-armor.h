#pragma once

#include "object-enchant/protector/apply-magic-armor.h"
#include "system/angband.h"

class CreatureEntity;
class ItemEntity;
class SoftArmorEnchanter : public ArmorEnchanter {
public:
    SoftArmorEnchanter(CreatureEntity &creature, ItemEntity *o_ptr, DEPTH level, int power);
    SoftArmorEnchanter(const SoftArmorEnchanter &) = default;
    SoftArmorEnchanter(SoftArmorEnchanter &&) = default;
    SoftArmorEnchanter &operator=(const SoftArmorEnchanter &) = delete;
    SoftArmorEnchanter &operator=(SoftArmorEnchanter &&) = delete;
    void apply_magic() override;

protected:
    void sval_enchant() override;
    void give_high_ego_index() override;

private:
    bool is_high_ego_generated = false;
};
