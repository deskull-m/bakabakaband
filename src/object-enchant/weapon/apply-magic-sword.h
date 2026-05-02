#pragma once

#include "object-enchant/weapon/melee-weapon-enchanter.h"
#include "system/angband.h"

class CreatureEntity;
class ItemEntity;
class SwordEnchanter : public MeleeWeaponEnchanter {
public:
    SwordEnchanter(CreatureEntity &creature, ItemEntity *o_ptr, DEPTH level, int power);
    SwordEnchanter &operator=(const SwordEnchanter &) = delete;
    SwordEnchanter &operator=(SwordEnchanter &&) = delete;

    void apply_magic() override;

protected:
    void decide_skip() override;
    void sval_enchant() override{};
    void give_ego_index() override;
    void give_high_ego_index() override{};
    void give_cursed() override;
};
