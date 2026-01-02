#pragma once

#include "system/angband.h"

class PlayerType;
class CreatureEntity;
void gain_exp_64(PlayerType *player_ptr, int32_t amount, uint32_t amount_frac);
void gain_exp(CreatureEntity &creature, int32_t amount);
bool restore_level(CreatureEntity &creature);
void lose_exp(CreatureEntity &creature, int32_t amount);
bool drain_exp(PlayerType *player_ptr, int32_t drain, int32_t slip, int hold_exp_prob);
