#pragma once

#include "object-enchant/protector/abstract-protector-enchanter.h"
#include "system/angband.h"

class CreatureEntity;
class ItemEntity;
class ShieldEnchanter : public AbstractProtectorEnchanter {
public:
    ShieldEnchanter(CreatureEntity &creature, ItemEntity *o_ptr, DEPTH level, int power);
    ShieldEnchanter(const ShieldEnchanter &) = default;
    ShieldEnchanter(ShieldEnchanter &&) = default;
    ShieldEnchanter &operator=(const ShieldEnchanter &) = delete;
    ShieldEnchanter &operator=(ShieldEnchanter &&) = delete;
    void apply_magic() override;

protected:
    void sval_enchant() override {};
    void give_ego_index() override;
    void give_high_ego_index() override {};
    void give_cursed() override {};

private:
    CreatureEntity &creature;
};
