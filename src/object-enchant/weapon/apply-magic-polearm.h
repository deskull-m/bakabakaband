#pragma once

#include "object-enchant/weapon/melee-weapon-enchanter.h"
#include "system/angband.h"

class CreatureEntity;
class ItemEntity;
class PolearmEnchanter : public MeleeWeaponEnchanter {
public:
    PolearmEnchanter(CreatureEntity &creature, ItemEntity *o_ptr, DEPTH level, int power);
    PolearmEnchanter(const PolearmEnchanter &) = default;
    PolearmEnchanter(PolearmEnchanter &&) = default;
    PolearmEnchanter &operator=(const PolearmEnchanter &) = delete;
    PolearmEnchanter &operator=(PolearmEnchanter &&) = delete;

    void apply_magic() override;

protected:
    void decide_skip() override;
    void sval_enchant() override {};
    void give_ego_index() override;
    void give_high_ego_index() override {};
    void give_cursed() override;
};
