#pragma once

#include "player-status/player-basic-statistics.h"

class CreatureEntity;
class PlayerWisdom : public PlayerBasicStatistics {
public:
    PlayerWisdom(CreatureEntity &creature);
    PlayerWisdom &operator=(const PlayerWisdom &) = delete;
    PlayerWisdom &operator=(PlayerWisdom &&) = delete;

protected:
    void set_locals() override;
    int16_t stance_bonus() override;
    int16_t mutation_bonus() override;
};
