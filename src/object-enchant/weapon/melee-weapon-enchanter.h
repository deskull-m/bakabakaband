#pragma once

#include "object-enchant/weapon/abstract-weapon-enchanter.h"
#include "system/angband.h"

class CreatureEntity;
class ItemEntity;
class MeleeWeaponEnchanter : public AbstractWeaponEnchanter {
public:
    MeleeWeaponEnchanter(const MeleeWeaponEnchanter &) = delete;
    MeleeWeaponEnchanter(MeleeWeaponEnchanter &&) = delete;
    MeleeWeaponEnchanter &operator=(const MeleeWeaponEnchanter &) = delete;
    MeleeWeaponEnchanter &operator=(MeleeWeaponEnchanter &&) = delete;
    virtual ~MeleeWeaponEnchanter() = default;

    void apply_magic() override;

protected:
    MeleeWeaponEnchanter(CreatureEntity &creature, ItemEntity *o_ptr, DEPTH level, int power);

    CreatureEntity &creature;

    void prepare_magic_application();

private:
    void strengthen();
};
