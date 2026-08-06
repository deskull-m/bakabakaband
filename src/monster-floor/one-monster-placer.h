#pragma once

#include "system/angband.h"
#include <tl/optional.hpp>

enum class MonraceId : int16_t;
class CreatureEntity;
tl::optional<MONSTER_IDX> place_monster_one(CreatureEntity &player, POSITION y, POSITION x, MonraceId r_idx, BIT_FLAGS mode, tl::optional<MONSTER_IDX> summoner_m_idx = tl::nullopt);
