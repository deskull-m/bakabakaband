#pragma once

#include <memory>

// Activation Execution.
class CreatureEntity;
class ItemEntity;
struct ae_type {
    int dir = 0;
    bool success = false;
    std::shared_ptr<ItemEntity> item = nullptr;
    int lev = 0;
    int chance = 0;
    int fail = 0;
    int broken = 0;

    ae_type(CreatureEntity &player, short i_idx);

    void decide_activation_level();
};
