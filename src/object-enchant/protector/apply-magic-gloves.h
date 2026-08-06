#pragma once

#include "object-enchant/protector/abstract-protector-enchanter.h"
#include "system/angband.h"

class CreatureEntity;
class ItemEntity;
class GlovesEnchanter : public AbstractProtectorEnchanter {
public:
    GlovesEnchanter(CreatureEntity &creature, ItemEntity *o_ptr, DEPTH level, int power);
    GlovesEnchanter(const GlovesEnchanter &) = default;
    GlovesEnchanter(GlovesEnchanter &&) = default;
    GlovesEnchanter &operator=(const GlovesEnchanter &) = delete;
    GlovesEnchanter &operator=(GlovesEnchanter &&) = delete;
    void apply_magic() override;

protected:
    void sval_enchant() override {};
    void give_ego_index() override {};
    void give_high_ego_index() override {};
    void give_cursed() override {};

private:
    CreatureEntity &creature;
};
