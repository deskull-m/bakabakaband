#pragma once

#include "object-enchant/protector/abstract-protector-enchanter.h"
#include "system/angband.h"

class CreatureEntity;
class ItemEntity;
class CrownEnchanter : public AbstractProtectorEnchanter {
public:
    CrownEnchanter(CreatureEntity &creature, ItemEntity *o_ptr, DEPTH level, int power);
    CrownEnchanter(const CrownEnchanter &) = default;
    CrownEnchanter(CrownEnchanter &&) = default;
    CrownEnchanter &operator=(const CrownEnchanter &) = delete;
    CrownEnchanter &operator=(CrownEnchanter &&) = delete;
    void apply_magic() override;

protected:
    void sval_enchant() override {};
    void give_ego_index() override;
    void give_high_ego_index() override {};
    void give_cursed() override;

private:
    CreatureEntity &creature;
};
