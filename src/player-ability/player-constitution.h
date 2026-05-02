#pragma once

#include "player-status/player-basic-statistics.h"

class CreatureEntity;
class PlayerConstitution : public PlayerBasicStatistics {
public:
    PlayerConstitution(CreatureEntity &creature);
    PlayerConstitution(const PlayerConstitution &) = delete;
    PlayerConstitution(PlayerConstitution &&) = delete;
    PlayerConstitution &operator=(const PlayerConstitution &) = delete;
    PlayerConstitution &operator=(PlayerConstitution &&) = delete;

protected:
    void set_locals() override;
    int16_t race_bonus() override;
    int16_t time_effect_bonus() override;
    int16_t stance_bonus() override;
    int16_t mutation_bonus() override;
};
