#pragma once

#include "player-status/player-basic-statistics.h"

class CreatureEntity;
class PlayerIntelligence : public PlayerBasicStatistics {
public:
    PlayerIntelligence(CreatureEntity &creature);
    PlayerIntelligence(const PlayerIntelligence &) = delete;
    PlayerIntelligence(PlayerIntelligence &&) = delete;
    PlayerIntelligence &operator=(const PlayerIntelligence &) = delete;
    PlayerIntelligence &operator=(PlayerIntelligence &&) = delete;

protected:
    void set_locals() override;
    int16_t stance_bonus() override;
    int16_t mutation_bonus() override;
};
