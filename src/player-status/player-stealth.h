#pragma once
#include "player-status/player-status-base.h"

class CreatureEntity;
class PlayerStealth : public PlayerStatusBase {
public:
    PlayerStealth(CreatureEntity &creature);
    PlayerStealth(const PlayerStealth &) = delete;
    PlayerStealth(PlayerStealth &&) = delete;
    PlayerStealth &operator=(const PlayerStealth &) = delete;
    PlayerStealth &operator=(PlayerStealth &&) = delete;

    BIT_FLAGS get_bad_flags() override;

protected:
    void set_locals() override;
    int16_t race_bonus() override;
    int16_t class_bonus() override;
    int16_t class_base_bonus() override;
    int16_t personality_bonus() override;
    int16_t time_effect_bonus() override;
    int16_t mutation_bonus() override;
    int16_t set_exception_bonus(int16_t value) override;
    bool is_aggravated_s_fairy();
};
