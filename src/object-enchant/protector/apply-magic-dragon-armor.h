#pragma once

#include "object-enchant/protector/abstract-protector-enchanter.h"
#include "system/angband.h"

class CreatureEntity;
class ItemEntity;
class DragonArmorEnchanter : public AbstractProtectorEnchanter {
public:
    DragonArmorEnchanter(CreatureEntity &creature, ItemEntity *o_ptr, DEPTH level, int power);
    DragonArmorEnchanter &operator=(const DragonArmorEnchanter &) = delete;
    DragonArmorEnchanter &operator=(DragonArmorEnchanter &&) = delete;
    void apply_magic() override;

protected:
    void sval_enchant() override{};
    void give_ego_index() override{};
    void give_high_ego_index() override{};
    void give_cursed() override{};

private:
    CreatureEntity &creature;
};
