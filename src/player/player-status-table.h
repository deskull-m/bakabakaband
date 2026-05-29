#pragma once

#include "player-ability/player-ability-types.h"
#include "system/angband.h"
#include "system/system-variables.h"
#include <array>

extern const std::array<byte, STAT_TABLE_SIZE> adj_mag_study;
extern const std::array<byte, STAT_TABLE_SIZE> adj_mag_mana;
extern const std::array<byte, STAT_TABLE_SIZE> adj_mag_fail;
extern const std::array<byte, STAT_TABLE_SIZE> adj_mag_stat;
extern const std::array<byte, STAT_TABLE_SIZE> adj_chr_gold;
extern const std::array<byte, STAT_TABLE_SIZE> adj_int_dev;
extern const std::array<byte, STAT_TABLE_SIZE> adj_wis_sav;
extern const std::array<byte, STAT_TABLE_SIZE> adj_dex_dis;
extern const std::array<byte, STAT_TABLE_SIZE> adj_int_dis;
extern const std::array<byte, STAT_TABLE_SIZE> adj_dex_ta;
extern const std::array<byte, STAT_TABLE_SIZE> adj_str_td;
extern const std::array<byte, STAT_TABLE_SIZE> adj_dex_th;
extern const std::array<byte, STAT_TABLE_SIZE> adj_str_th;
extern const std::array<byte, STAT_TABLE_SIZE> adj_str_wgt;
extern const std::array<byte, STAT_TABLE_SIZE> adj_str_hold;
extern const std::array<byte, STAT_TABLE_SIZE> adj_str_dig;
extern const std::array<byte, STAT_TABLE_SIZE> adj_dex_safe;
extern const std::array<byte, STAT_TABLE_SIZE> adj_con_fix;
extern const std::array<byte, STAT_TABLE_SIZE> adj_con_mhp;
extern const std::array<byte, STAT_TABLE_SIZE> adj_chr_chm;

extern const concptr stat_names[A_MAX];
extern const concptr stat_names_reduced[A_MAX];

extern const int32_t player_exp[PY_MAX_LEVEL];
extern const int32_t player_exp_a[PY_MAX_LEVEL];
