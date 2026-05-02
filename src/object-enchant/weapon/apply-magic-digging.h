#pragma once

#include "object-enchant/weapon/abstract-weapon-enchanter.h"
#include "system/angband.h"

class CreatureEntity;
class ItemEntity;
class DiggingEnchanter : public AbstractWeaponEnchanter {
public:
    DiggingEnchanter(CreatureEntity &creature, ItemEntity *o_ptr, DEPTH level, int power);
    DiggingEnchanter &operator=(const DiggingEnchanter &) = delete;
    DiggingEnchanter &operator=(DiggingEnchanter &&) = delete;
    void apply_magic() override;

protected:
    void sval_enchant() override{};
    void give_ego_index() override{};
    void give_high_ego_index() override{};
    void give_cursed() override{};

private:
    CreatureEntity &creature;
};
