#pragma once

#include "system/creature-entity.h"
#include <string_view>

class PlayerType : public CreatureEntity {
public:
    PlayerType();

    bool is_valid() const override;
    bool is_dead() const override;

    bool is_player() const override;

    void on_take_hit(int damage) override;
    void on_death(std::string_view cause) override;
    bool calc_damage_reduction(int &damage, int damage_type) override;

    short get_timed_effect(CreatureTimedEffect effect) const override;
    void set_timed_effect(CreatureTimedEffect effect, short value) override;

    void wipe() override;
};

extern PlayerType *p_ptr;
