#pragma once

#include "player-ability/player-ability-types.h"
#include "system/angband.h"
#include "system/system-variables.h"
#include <array>

extern const int monk_ave_damage[PY_MAX_LEVEL + 1][3];
extern const std::array<byte, STAT_TABLE_SIZE> adj_str_blow;
extern const std::array<byte, STAT_TABLE_SIZE> adj_dex_blow;
extern const byte blows_table[12][12];
