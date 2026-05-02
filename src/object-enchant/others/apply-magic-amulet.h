#pragma once

#include "object-enchant/enchanter-base.h"
#include "system/angband.h"

class CreatureEntity;
class ItemEntity;
class AmuletEnchanter : public EnchanterBase {
public:
    AmuletEnchanter(CreatureEntity &creature, ItemEntity *o_ptr, DEPTH level, int power);
    AmuletEnchanter &operator=(const AmuletEnchanter &) = delete;
    AmuletEnchanter &operator=(AmuletEnchanter &&) = delete;
    virtual ~AmuletEnchanter() = default;
    void apply_magic() override;

protected:
    void sval_enchant() override;
    void give_ego_index() override;
    void give_high_ego_index() override;
    void give_cursed() override;

private:
    CreatureEntity &creature;
    ItemEntity *o_ptr;
    DEPTH level;
    int power;
};
