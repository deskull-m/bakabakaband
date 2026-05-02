#pragma once

#include "player-status/player-basic-statistics.h"

class CreatureEntity;
class PlayerCharisma : public PlayerBasicStatistics {
public:
    PlayerCharisma(CreatureEntity &creature);
    PlayerCharisma(const PlayerCharisma &) = delete;
    PlayerCharisma(PlayerCharisma &&) = delete;
    PlayerCharisma &operator=(const PlayerCharisma &) = delete;
    PlayerCharisma &operator=(PlayerCharisma &&) = delete;

    BIT_FLAGS get_all_flags() override;
    BIT_FLAGS get_bad_flags() override;

protected:
    void set_locals() override;
    int16_t stance_bonus() override;
    int16_t mutation_bonus() override;
    int16_t set_exception_bonus(int16_t value) override;
    int16_t set_exception_use_status(int16_t value) override;
};
