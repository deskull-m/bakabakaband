#pragma once

#include "system/angband.h"
#include "util/point-2d.h"
#include <tl/optional.hpp>

enum summon_type : int;
enum class MonraceId : int16_t;
class PlayerType;
using summon_specific_pf = tl::optional<MONSTER_IDX>(PlayerType *, POSITION, POSITION, DEPTH, summon_type, BIT_FLAGS, tl::optional<MONSTER_IDX>);

tl::optional<Pos2D> mon_scatter(PlayerType *player_ptr, MonraceId monrace_id, const Pos2D &pos, int max_distance);
tl::optional<MONSTER_IDX> multiply_monster(PlayerType *player_ptr, MONSTER_IDX m_idx, MonraceId r_idx, bool clone, BIT_FLAGS mode);
tl::optional<MONSTER_IDX> place_specific_monster(PlayerType *player_ptr, POSITION y, POSITION x, MonraceId r_idx, BIT_FLAGS mode, tl::optional<MONSTER_IDX> summoner_m_idx = tl::nullopt);
tl::optional<MONSTER_IDX> place_random_monster(PlayerType *player_ptr, POSITION y, POSITION x, BIT_FLAGS mode);
bool alloc_horde(PlayerType *player_ptr, POSITION y, POSITION x, summon_specific_pf summon_specific);
bool alloc_guardian(PlayerType *player_ptr, bool def_val);
bool alloc_monster(PlayerType *player_ptr, int min_dis, BIT_FLAGS mode, summon_specific_pf summon_specific, int max_dis = 65535);
