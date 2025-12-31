#pragma once

#include <cstdint>
#include <string>
#include <tl/optional.hpp>

class PlayerType;

/*!
 * @brief ダンジョンID・レベルから地形生成用のシード値を計算する
 * @param dungeon_id ダンジョンID
 * @param dun_level ダンジョン階層
 * @return 計算されたシード値
 * @details 同じダンジョンの同じ階層では常に同じシード値を返す
 */
uint32_t calculate_terrain_seed(int dungeon_id, int dun_level);

tl::optional<std::string> cave_gen(PlayerType *player_ptr, tl::optional<uint32_t> seed = tl::nullopt);
void apply_vestige_terrain_replacement(PlayerType *player_ptr);
void apply_void_terrain_placement(PlayerType *player_ptr);
