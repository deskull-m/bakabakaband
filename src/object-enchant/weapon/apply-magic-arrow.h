#pragma once

#include "object-enchant/weapon/abstract-weapon-enchanter.h"
#include "system/angband.h"

class CreatureEntity;
class ItemEntity;
class ArrowEnchanter : public AbstractWeaponEnchanter {
public:
    ArrowEnchanter(CreatureEntity &creature, ItemEntity *o_ptr, DEPTH level, int power);
    ArrowEnchanter(const ArrowEnchanter &) = delete;
    ArrowEnchanter(ArrowEnchanter &&) = delete;
    ArrowEnchanter &operator=(const ArrowEnchanter &) = delete;
    ArrowEnchanter &operator=(ArrowEnchanter &&) = delete;
    void apply_magic() override;

protected:
    void sval_enchant() override{};
    void give_ego_index() override{};
    void give_high_ego_index() override{};
    void give_cursed() override{};

private:
    CreatureEntity &creature;
};
