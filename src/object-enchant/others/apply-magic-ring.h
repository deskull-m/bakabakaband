#pragma once

#include "object-enchant/enchanter-base.h"
#include "system/angband.h"

class CreatureEntity;
class ItemEntity;
class RingEnchanter : public EnchanterBase {
public:
    RingEnchanter(CreatureEntity &creature, ItemEntity *o_ptr, DEPTH level, int power);
    RingEnchanter(const RingEnchanter &) = default;
    RingEnchanter(RingEnchanter &&) = default;
    RingEnchanter &operator=(const RingEnchanter &) = delete;
    RingEnchanter &operator=(RingEnchanter &&) = delete;
    virtual ~RingEnchanter() = default;
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
