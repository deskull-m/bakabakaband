#pragma once

#include "object-enchant/enchanter-base.h"
#include "system/angband.h"

class CreatureEntity;
class ItemEntity;
class LiteEnchanter : public EnchanterBase {
public:
    LiteEnchanter(CreatureEntity &creature, ItemEntity *o_ptr, int power);
    LiteEnchanter &operator=(const LiteEnchanter &) = delete;
    LiteEnchanter &operator=(LiteEnchanter &&) = delete;
    void apply_magic() override;

protected:
    void sval_enchant() override{};
    void give_ego_index() override;
    void give_high_ego_index() override{};
    void give_cursed() override;

private:
    CreatureEntity &creature;
    ItemEntity *o_ptr;
    int power;

    void add_dark_flag();
};
