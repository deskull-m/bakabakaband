#pragma once

#include "object-enchant/weapon/melee-weapon-enchanter.h"
#include "system/angband.h"

class CreatureEntity;
class ItemEntity;
class HaftedEnchanter : public MeleeWeaponEnchanter {
public:
    HaftedEnchanter(CreatureEntity &creature, ItemEntity *o_ptr, DEPTH level, int power);
    HaftedEnchanter &operator=(const HaftedEnchanter &) = delete;
    HaftedEnchanter &operator=(HaftedEnchanter &&) = delete;

    void apply_magic() override;

protected:
    void sval_enchant() override{};
    void give_ego_index() override;
    void give_high_ego_index() override{};
    void give_cursed() override;
};
