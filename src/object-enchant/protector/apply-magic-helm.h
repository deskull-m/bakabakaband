#pragma once

#include "object-enchant/protector/abstract-protector-enchanter.h"
#include "system/angband.h"

class CreatureEntity;
class ItemEntity;
class HelmEnchanter : public AbstractProtectorEnchanter {
public:
    HelmEnchanter(CreatureEntity &creature, ItemEntity *o_ptr, DEPTH level, int power);
    HelmEnchanter(const HelmEnchanter &) = delete;
    HelmEnchanter(HelmEnchanter &&) = delete;
    HelmEnchanter &operator=(const HelmEnchanter &) = delete;
    HelmEnchanter &operator=(HelmEnchanter &&) = delete;
    void apply_magic() override;

protected:
    void sval_enchant() override{};
    void give_ego_index() override;
    void give_high_ego_index() override{};
    void give_cursed() override;

private:
    CreatureEntity &creature;
};
