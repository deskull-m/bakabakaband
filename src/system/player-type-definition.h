#pragma once

#include "combat/martial-arts-style.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/trc-types.h"
#include "player-ability/player-ability-types.h"
#include "player-info/class-specific-data.h"
#include "player-info/class-types.h"
#include "player-info/race-types.h"
#include "player/player-personality-types.h"
#include "player/player-sex.h"
#include "system/angband.h"
#include "system/creature-entity.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/system-variables.h"
#include "util/dice.h"
#include "util/flag-group.h"
#include "util/point-2d.h"
#include <array>
#include <map>
#include <string>
#include <string_view>

class PlayerType : public CreatureEntity {
public:
    PlayerType();
    void plus_incident(INCIDENT incidentID, int num);
    bool is_true_winner() const;

    void ride_monster(MONSTER_IDX m_idx);
    bool is_fully_healthy() const;
    bool is_located_at_running_destination() const;
    bool try_set_position(const Pos2D &pos);
    void set_position(const Pos2D &pos);
    bool in_saved_floor() const;
    bool try_resist_eldritch_horror() const;

    bool is_valid() const override;
    bool is_dead() const override;

    bool is_player() const override;

    void on_take_hit(int damage) override;
    void on_death(std::string_view cause) override;

    short get_timed_effect(CreatureTimedEffect effect) const override;
    void set_timed_effect(CreatureTimedEffect effect, short value) override;
};

extern PlayerType *p_ptr;
