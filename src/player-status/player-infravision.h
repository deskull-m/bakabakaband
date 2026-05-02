#pragma once
#include "player-status/player-status-base.h"

class CreatureEntity;
class PlayerInfravision : public PlayerStatusBase {
public:
    PlayerInfravision(CreatureEntity &creature);
    PlayerInfravision &operator=(const PlayerInfravision &) = delete;
    PlayerInfravision &operator=(PlayerInfravision &&) = delete;

protected:
    void set_locals() override;
    int16_t race_bonus() override;
    int16_t time_effect_bonus() override;
    int16_t mutation_bonus() override;
};
