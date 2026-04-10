#pragma once

#include "system/creature-entity.h"
#include <string_view>

class PlayerType : public CreatureEntity {
public:
    PlayerType();
    void plus_incident(INCIDENT incidentID, int num);

    void set_position(const Pos2D &pos);

    bool is_valid() const override;
    bool is_dead() const override;

    bool is_player() const override;

    void on_take_hit(int damage) override;
    void on_death(std::string_view cause) override;

    short get_timed_effect(CreatureTimedEffect effect) const override;
    void set_timed_effect(CreatureTimedEffect effect, short value) override;

};

extern PlayerType *p_ptr;
