#pragma once

#include "system/creature-entity.h"
#include "system/monster-profile.h"

/*!
 * @brief Monster information, for a specific monster.
 * @Note
 * fy, fx constrain dungeon size to 256x256
 * The "hold_o_idx" field points to the first object of a stack
 * of objects (if any) being carried by the monster (see above).
 */
class MonsterEntityWriter;
class MonsterEntity : public CreatureEntity {
public:
    friend class MonsterEntityWriter;
    MonsterEntity();
    MonsterEntity(MonsterEntity &&) = default;
    MonsterEntity &operator=(MonsterEntity &&) = default;
    MonsterEntity(const MonsterEntity &) = default;
    MonsterEntity &operator=(const MonsterEntity &) = default;

    void wipe() override;
    MonsterEntity clone() const;
    bool can_ring_boss_call_nazgul() const;
    void set_individual_speed(bool force_fixed_speed) override;

    int get_level() const override;
};
