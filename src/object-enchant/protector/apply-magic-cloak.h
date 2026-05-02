#pragma once

#include "object-enchant/protector/abstract-protector-enchanter.h"
#include "system/angband.h"

class CreatureEntity;
class ItemEntity;
class CloakEnchanter : public AbstractProtectorEnchanter {
public:
    CloakEnchanter(CreatureEntity &creature, ItemEntity *o_ptr, DEPTH level, int power);
    CloakEnchanter &operator=(const CloakEnchanter &) = delete;
    CloakEnchanter &operator=(CloakEnchanter &&) = delete;
    void apply_magic() override;

protected:
    void sval_enchant() override{};
    void give_ego_index() override{};
    void give_high_ego_index() override{};
    void give_cursed() override{};

private:
    CreatureEntity &creature;
};
