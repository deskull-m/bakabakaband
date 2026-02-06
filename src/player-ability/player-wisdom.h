#pragma once

#include "player-status/player-basic-statistics.h"

class CreatureEntity;
class PlayerWisdom : public PlayerBasicStatistics {
public:
    PlayerWisdom(CreatureEntity &creature);

protected:
    void set_locals() override;
    int16_t stance_bonus() override;
    int16_t mutation_bonus() override;
};
