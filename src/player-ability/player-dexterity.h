#pragma once

#include "player-status/player-basic-statistics.h"

class CreatureEntity;
class PlayerDexterity : public PlayerBasicStatistics {
public:
    PlayerDexterity(CreatureEntity &creature);
    PlayerDexterity &operator=(const PlayerDexterity &) = delete;
    PlayerDexterity &operator=(PlayerDexterity &&) = delete;

protected:
    void set_locals() override;
    int16_t race_bonus() override;
    int16_t time_effect_bonus() override;
    int16_t stance_bonus() override;
    int16_t mutation_bonus() override;
};
