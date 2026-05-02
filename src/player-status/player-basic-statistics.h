#pragma once

#include "player-ability/player-ability-types.h"
#include "player-status/player-status-base.h"

class CreatureEntity;
class PlayerBasicStatistics : public PlayerStatusBase {
public:
    PlayerBasicStatistics(const PlayerBasicStatistics &) = delete;
    PlayerBasicStatistics(PlayerBasicStatistics &&) = delete;
    PlayerBasicStatistics &operator=(const PlayerBasicStatistics &) = delete;
    PlayerBasicStatistics &operator=(PlayerBasicStatistics &&) = delete;

    void update_value();
    int16_t modification_value();
    int16_t get_value() override;

protected:
    PlayerBasicStatistics(CreatureEntity &creature);

    player_ability_type ability_type{};
    int16_t race_bonus() override;
    int16_t class_bonus() override;
    int16_t personality_bonus() override;
    void update_top_status();
    void update_use_status();
    void update_index_status();
    virtual int16_t set_exception_use_status(int16_t value);
};
