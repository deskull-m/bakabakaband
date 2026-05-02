#pragma once

#include "object-enchant/enchanter-base.h"

class CreatureEntity;
class ItemEntity;
class OtherItemsEnchanter : public EnchanterBase {
public:
    OtherItemsEnchanter(CreatureEntity &creature, ItemEntity *o_ptr);
    OtherItemsEnchanter(const OtherItemsEnchanter &) = delete;
    OtherItemsEnchanter(OtherItemsEnchanter &&) = delete;
    OtherItemsEnchanter &operator=(const OtherItemsEnchanter &) = delete;
    OtherItemsEnchanter &operator=(OtherItemsEnchanter &&) = delete;
    void apply_magic() override;

    void sval_enchant() override{};
    void give_ego_index() override{};
    void give_high_ego_index() override{};
    void give_cursed() override{};

private:
    CreatureEntity &creature;
    ItemEntity *o_ptr;

    void enchant_wand_staff();
    void generate_figurine();
    void generate_corpse();
    void generate_statue();
    void generate_chest();
    void generate_disarmed_trap();
};
