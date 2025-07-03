#pragma once

#include "system/angband.h"

enum summon_type : int;
enum class MonraceId : int16_t;
class PlayerType;
bool check_summon_specific(PlayerType *player_ptr, MonraceId summoner_idx, MonraceId r_idx, summon_type type);
