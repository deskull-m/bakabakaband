#pragma once

#include "floor/floor-allocation-types.h"
#include "system/angband.h"
#include "system/creature-entity.h"
#include "util/point-2d.h"
#include <tl/optional.hpp>

/*!
 * @brief フロアの 4 辺。階段を特定の辺へ寄せる指定に使う (Issue: 雪原・砂漠の対向配置階段)
 */
enum class FloorEdge {
    NORTH, //!< 上辺 (y が小さい側)
    SOUTH, //!< 下辺 (y が大きい側)
    WEST, //!< 左辺 (x が小さい側)
    EAST, //!< 右辺 (x が大きい側)
};

FloorEdge get_opposite_edge(FloorEdge edge);
FloorEdge choose_random_floor_edge();
int calc_edge_band_depth(int extent);
bool is_within_edge_band(const Pos2D &pos, int height, int width, FloorEdge edge);

enum dap_type : int;
bool can_place_stair_at(const CreatureEntity &creature, const Pos2D &pos, int walls);
bool alloc_stairs(CreatureEntity &creature, FEAT_IDX feat, int num, int walls, tl::optional<FloorEdge> edge = tl::nullopt);
void alloc_object(CreatureEntity &creature, dap_type set, dungeon_allocation_type typ, int num);
void alloc_specific_floor_items(CreatureEntity &creature);
