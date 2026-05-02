#pragma once

#include "object-enchant/weapon/abstract-weapon-enchanter.h"
#include "system/angband.h"

class CreatureEntity;
class ItemEntity;
class BowEnchanter : public AbstractWeaponEnchanter {
public:
    BowEnchanter(CreatureEntity &creature, ItemEntity *o_ptr, DEPTH level, int power);
    BowEnchanter &operator=(const BowEnchanter &) = delete;
    BowEnchanter &operator=(BowEnchanter &&) = delete;
    void apply_magic() override;

protected:
    void sval_enchant() override{};
    void give_ego_index() override{};
    void give_high_ego_index() override{};
    void give_cursed() override{};

private:
    CreatureEntity &creature;
};
