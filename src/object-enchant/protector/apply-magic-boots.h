#pragma once

#include "object-enchant/protector/abstract-protector-enchanter.h"
#include "system/angband.h"

class CreatureEntity;
class ItemEntity;
class BootsEnchanter : public AbstractProtectorEnchanter {
public:
    BootsEnchanter(CreatureEntity &creature, ItemEntity *o_ptr, DEPTH level, int power);
    BootsEnchanter(const BootsEnchanter &) = default;
    BootsEnchanter(BootsEnchanter &&) = default;
    BootsEnchanter &operator=(const BootsEnchanter &) = delete;
    BootsEnchanter &operator=(BootsEnchanter &&) = delete;
    void apply_magic() override;

protected:
    void sval_enchant() override{};
    void give_ego_index() override{};
    void give_high_ego_index() override{};
    void give_cursed() override{};

private:
    CreatureEntity &creature;
};
