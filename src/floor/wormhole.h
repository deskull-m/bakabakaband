/*!
 * @file wormhole.h
 * @brief 同一フロアの別地点へ相互に繋がるワームホール地形
 */

#pragma once

#include "system/angband.h"
#include "util/point-2d.h"
#include <tl/optional.hpp>
#include <vector>

/*!
 * @brief 最大面積のフロアでワームホールが生成される確率 [%]
 * @details 出現率は面積に比例させる (広いフロアほど出やすい)。狭いフロアでは
 *          この値を面積比で割った値になるため、最小フロア (45x45) では 5%。
 *          バランス調整はこの定数で行う。
 */
constexpr auto WORMHOLE_MAX_AREA_CHANCE_PERCENT = 50;

/*!
 * @brief この面積ごとにワームホールの対が 1 組増える
 * @details 最小でも 1 組は置くので、最大面積のフロア (132x132 = 17424) で 2 組になる。
 */
constexpr auto WORMHOLE_AREA_PER_PAIR = 9000;

/*!
 * @brief ワームホールを置く床に要求する隣接外壁数 (階段と同じ初期値)
 */
constexpr auto WORMHOLE_REQUIRED_WALLS = 3;

class CreatureEntity;
class FloorType;

int calc_wormhole_appearance_chance(int height, int width);
int calc_wormhole_pairs(int height, int width);
tl::optional<int> calc_wormhole_partner_index(int count, int index);

std::vector<Pos2D> collect_wormholes(const FloorType &floor);
tl::optional<Pos2D> find_wormhole_partner(const FloorType &floor, const Pos2D &pos);

bool alloc_wormholes(CreatureEntity &creature, int pairs, int walls);
void alloc_wormholes_by_floor_area(CreatureEntity &creature);
void do_cmd_enter_wormhole(CreatureEntity &creature);
