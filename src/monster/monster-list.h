#pragma once

#include "alliance/alliance.h"
#include <cstdint>
#include <tl/optional.hpp>

enum class MonraceId : int16_t;
class FloorType;
class MonraceDefinition;
class PlayerType;
MonraceId get_mon_num(PlayerType *player_ptr, int min_level, int max_level, uint32_t mode, AllianceType alliance_type = AllianceType::NONE);
void choose_chameleon_polymorph(PlayerType *player_ptr, short m_idx, short terrain_id, tl::optional<short> summoner_m_idx = tl::nullopt);
int get_monster_crowd_number(const FloorType &floor, short m_idx);
