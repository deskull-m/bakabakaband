#include "floor/object-allocator.h"
#include "dungeon/quest.h"
#include "floor/floor-generator-util.h"
#include "floor/floor-object.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-types.h"
#include "grid/object-placer.h"
#include "object-enchant/item-apply-magic.h"
#include "object/object-kind-hook.h"
#include "system/creature-entity.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/quest-definition.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "term/z-rand.h"
#include "wizard/wizard-messages.h"
#include <algorithm>
#include <array>
#include <range/v3/view.hpp>

/*!
 * @brief 上下左右の外壁数をカウントする
 * @param pos 基準座標
 * @return 隣接する外壁の数
 */
static int next_to_walls(const FloorType &floor, const Pos2D &pos)
{
    auto k = 0;
    for (const auto &d : Direction::directions_4()) {
        const auto pos_neighbor = pos + d.vec();
        if (floor.contains(pos_neighbor, FloorBoundary::OUTER_WALL_EXCLUSIVE) && floor.get_grid(pos_neighbor).is_extra()) {
            k++;
        }
    }

    return k;
}

/*!
 * @brief alloc_stairs()の補助として指定の位置に階段を生成できるかの判定を行う
 * @param creature クリーチャーへの参照
 * @param pos 基準座標
 * @param walls 最低減隣接させたい外壁の数
 * @return 階段を生成して問題がないならばTRUEを返す。
 */
static bool alloc_stairs_aux(const CreatureEntity &creature, const Pos2D &pos, int walls)
{
    const auto &floor = *creature.get_floor();
    const auto &grid = floor.get_grid(pos);
    if (!grid.is_floor() || grid.has(TerrainCharacteristics::PATTERN) || !grid.o_idx_list.empty() || grid.has_monster() || next_to_walls(floor, pos) < walls) {
        return false;
    }

    return true;
}

/*!
 * @brief 指定の辺の反対側の辺を返す
 * @param edge 基準の辺
 * @return 正反対の辺
 */
FloorEdge get_opposite_edge(FloorEdge edge)
{
    switch (edge) {
    case FloorEdge::NORTH:
        return FloorEdge::SOUTH;
    case FloorEdge::SOUTH:
        return FloorEdge::NORTH;
    case FloorEdge::WEST:
        return FloorEdge::EAST;
    case FloorEdge::EAST:
    default:
        return FloorEdge::WEST;
    }
}

/*!
 * @brief フロアの 4 辺から 1 つを無作為に選ぶ
 * @return 選ばれた辺
 */
FloorEdge choose_random_floor_edge()
{
    constexpr std::array edges = { FloorEdge::NORTH, FloorEdge::SOUTH, FloorEdge::WEST, FloorEdge::EAST };
    return rand_choice(edges);
}

/*!
 * @brief 辺に寄せる帯の厚みを算出する
 * @param extent フロアの当該方向の長さ
 * @return 帯の厚み (最低 1)
 * @details 厚みは辺の長さの `1 / STAIR_EDGE_BAND_DIVISOR`。
 *          「集中」の度合いはこの定数で調整する (大きくするほど狭くなる)。
 */
int calc_edge_band_depth(int extent)
{
    constexpr auto STAIR_EDGE_BAND_DIVISOR = 4;
    return std::max(1, extent / STAIR_EDGE_BAND_DIVISOR);
}

/*!
 * @brief 指定座標が当該の辺に寄せた帯の中にあるか
 * @param pos 判定する座標
 * @param height フロアの高さ
 * @param width フロアの幅
 * @param edge 寄せる辺
 * @return 帯の中ならtrue
 */
bool is_within_edge_band(const Pos2D &pos, int height, int width, FloorEdge edge)
{
    switch (edge) {
    case FloorEdge::NORTH:
        return pos.y <= calc_edge_band_depth(height);
    case FloorEdge::SOUTH:
        return pos.y >= height - 1 - calc_edge_band_depth(height);
    case FloorEdge::WEST:
        return pos.x <= calc_edge_band_depth(width);
    case FloorEdge::EAST:
    default:
        return pos.x >= width - 1 - calc_edge_band_depth(width);
    }
}

/*!
 * @brief 外壁に隣接させて階段を生成する / Places some staircases near walls
 * @param creature クリーチャーへの参照
 * @param feat 配置したい地形ID
 * @param num 配置したい階段の数
 * @param walls 最低減隣接させたい外壁の数
 * @param edge 指定するとその辺へ寄せた帯の中を優先して配置する (`POLARIZED_STAIRS` 用)
 * @return 規定数通りに生成に成功したらTRUEを返す。
 */
bool alloc_stairs(CreatureEntity &creature, FEAT_IDX feat, int num, int walls, tl::optional<FloorEdge> edge)
{
    int shaft_num = 0;
    const auto &terrain = TerrainList::get_instance().get_terrain(feat);
    auto &floor = *creature.get_floor();
    const auto &dungeon = floor.get_dungeon_definition();
    if (terrain.flags.has(TerrainCharacteristics::UP_STAIRS)) {
        if (ironman_downward || !floor.is_underground()) {
            return true;
        }

        if (floor.dun_level > dungeon.mindepth) {
            shaft_num = (randint1(num + 1)) / 2;
        }
    } else if (terrain.flags.has(TerrainCharacteristics::DOWN_STAIRS)) {
        auto quest_id = floor.get_quest_id();
        const auto &quests = QuestList::get_instance();
        if (floor.dun_level > 1 && inside_quest(quest_id)) {
            const auto &monrace = quests.get_quest(quest_id).get_bounty();
            if (!monrace.is_dead_unique()) {
                return true;
            }
        }

        if (floor.dun_level >= dungeon.maxdepth) {
            return true;
        }

        if ((floor.dun_level < dungeon.maxdepth - 1) && !inside_quest(floor.get_quest_id(1))) {
            shaft_num = (randint1(num) + 1) / 2;
        }
    } else {
        return false;
    }

    const auto walls_initial = walls;
    for (auto i = 0; i < num; i++) {
        // 辺の指定がある場合は帯の中を優先する。`edge` が無ければ従来どおりフロア全体から選ぶ。
        auto restricts_to_band = edge.has_value();
        while (true) {
            const auto can_alloc_stair = [&](const Pos2D &pos) {
                if (restricts_to_band && !is_within_edge_band(pos, floor.height, floor.width, *edge)) {
                    return false;
                }

                return alloc_stairs_aux(creature, pos, walls);
            };
            const auto pos_candidates =
                floor.get_area(FloorBoundary::OUTER_WALL_INCLUSIVE) |
                ranges::views::filter(can_alloc_stair) |
                ranges::to_vector;

            if (pos_candidates.empty()) {
                if (walls > 0) {
                    walls--;
                    continue;
                }

                if (restricts_to_band) {
                    // 帯の中に置ける床が無いならフロア全体へ逃がす。
                    // ここで諦めるとフロア生成そのものが失敗するため、帯は「寄せる」だけの指定にする。
                    restricts_to_band = false;
                    walls = walls_initial;
                    continue;
                }

                return false;
            }

            const auto &pos_stair = rand_choice(pos_candidates);
            auto &grid = floor.get_grid(pos_stair);
            grid.mimic = 0;
            grid.feat = (i < shaft_num) ? dungeon.convert_terrain_id(feat, TerrainCharacteristics::SHAFT) : feat;
            grid.info &= ~(CAVE_FLOOR);
            break;
        }
    }

    return true;
}

/*!
 * @brief フロア上のランダム位置に各種オブジェクトを配置する / Allocates some objects (using "place" and "type")
 * @param set 配置したい地形の種類
 * @param typ 配置したいオブジェクトの種類
 * @param num 配置したい数
 * @return 規定数通りに生成に成功したらTRUEを返す。
 */
void alloc_object(CreatureEntity &creature, dap_type set, dungeon_allocation_type typ, int num)
{
    auto dummy = 0;
    auto &floor = *creature.get_floor();
    num = num * floor.height * floor.width / (MAX_HGT * MAX_WID) + 1;
    for (auto k = 0; k < num; k++) {
        Pos2D pos(0, 0);
        while (dummy < SAFE_MAX_ATTEMPTS) {
            dummy++;
            pos.y = randint0(floor.height);
            pos.x = randint0(floor.width);
            const auto &grid = floor.get_grid(pos);
            if (!grid.is_floor() || !grid.o_idx_list.empty() || grid.has_monster()) {
                continue;
            }

            if (creature.is_located_at(pos)) {
                continue;
            }

            auto is_room = grid.is_room();
            if (((set == ALLOC_SET_CORR) && is_room) || ((set == ALLOC_SET_ROOM) && !is_room)) {
                continue;
            }

            break;
        }

        if (dummy >= SAFE_MAX_ATTEMPTS) {
            msg_print_wizard(creature, CHEAT_DUNGEON, _("アイテムの配置に失敗しました。", "Failed to place object."));
            return;
        }

        switch (typ) {
        case ALLOC_TYP_RUBBLE:
            floor.set_terrain_id_at(pos, TerrainTag::RUBBLE);
            floor.get_grid(pos).info &= ~(CAVE_FLOOR);
            break;
        case ALLOC_TYP_TRAP:
            floor.place_trap_at(pos);
            floor.get_grid(pos).info &= ~(CAVE_FLOOR);
            break;
        case ALLOC_TYP_GOLD:
            place_gold(creature, pos);
            break;
        case ALLOC_TYP_OBJECT:
            place_object(creature, pos, 0);
            break;
        case ALLOC_TYP_SUSHI:
            place_object(creature, pos, AM_IGNORE_LEVEL, kind_is_sushi);
            break;
        case ALLOC_TYP_SPECIFIC_ITEMS:
            // This case is handled by alloc_specific_floor_items function
            break;
        default:
            break;
        }
    }
}

/*!
 * @brief 特定階層でのダイスベースアイテム生成 / Generate items on specific floors using dice rules
 * @param creature クリーチャーへの参照
 */
void alloc_specific_floor_items(CreatureEntity &creature)
{
    auto &floor = *creature.get_floor();
    const auto &dungeon = floor.get_generated_dungeon_definition();

    // 現在の階層に特定のアイテム生成ルールがあるかチェック
    if (!dungeon.specific_item_generation_map.count(floor.dun_level)) {
        return;
    }

    const auto &rule = dungeon.specific_item_generation_map.at(floor.dun_level);

    // 確率チェック
    if (!evaluate_percent(rule.probability)) {
        return;
    }

    // ダイスロールで生成数を決定
    const int num_items = rule.dice.roll();

    // アイテムを生成
    for (int i = 0; i < num_items; i++) {
        // 特定のアイテムIDでアイテムを作成
        ItemEntity item(static_cast<short>(rule.item_id));
        item.generate(static_cast<short>(rule.item_id));

        // 適切な位置を探してアイテムを配置
        int dummy = 0;
        Pos2D pos(0, 0);

        while (dummy < SAFE_MAX_ATTEMPTS) {
            dummy++;
            pos.y = randint0(floor.height);
            pos.x = randint0(floor.width);
            const auto &grid = floor.get_grid(pos);

            if (!grid.is_floor() || !grid.o_idx_list.empty() || grid.has_monster()) {
                continue;
            }

            if (creature.is_located_at(pos)) {
                continue;
            }

            // アイテムを床に配置
            drop_near(creature, item, pos, false);
            break;
        }

        if (dummy >= SAFE_MAX_ATTEMPTS) {
            msg_print_wizard(creature, CHEAT_DUNGEON,
                _("特定階層アイテムの配置に失敗しました。", "Failed to place specific floor item."));
        }
    }
}
