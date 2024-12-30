#pragma once

#include "system/angband.h"
#include <optional>

enum class MonraceId : int16_t;
class PlayerType;
std::optional<MONSTER_IDX> place_monster_one(PlayerType *player_ptr, POSITION y, POSITION x, MonraceId r_idx, BIT_FLAGS mode, std::optional<MONSTER_IDX> summoner_m_idx = std::nullopt);
