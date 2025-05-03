#include "floor/cave-generator.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/quest-monster-placer.h"
#include "floor/dungeon-tunnel-util.h"
#include "floor/floor-allocation-types.h"
#include "floor/floor-streams.h"
#include "floor/geometry.h"
#include "floor/object-allocator.h"
#include "floor/tunnel-generator.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-types.h"
#include "game-option/game-play-options.h"
#include "grid/door.h"
#include "grid/feature-generator.h"
#include "grid/grid.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-util.h"
#include "room/lake-types.h"
#include "room/room-generator.h"
#include "room/rooms-maze-vault.h"
#include "system/dungeon/dungeon-data-definition.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "util/bit-flags-calculator.h"
#include "wizard/wizard-messages.h"

static void reset_lite_area(FloorType &floor)
{
    floor.lite_n = 0;
    floor.mon_lite_n = 0;
    floor.redraw_n = 0;
    floor.view_n = 0;
}

static void check_arena_floor(PlayerType *player_ptr, DungeonData *dd_ptr)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    if (!dd_ptr->empty_level) {
        for (POSITION y = 0; y < floor.height; y++) {
            for (POSITION x = 0; x < floor.width; x++) {
                place_bold(player_ptr, y, x, GB_EXTRA);
            }
        }

        return;
    }

    for (POSITION y = 0; y < floor.height; y++) {
        for (POSITION x = 0; x < floor.width; x++) {
            place_bold(player_ptr, y, x, GB_FLOOR);
        }
    }

    for (POSITION x = 0; x < floor.width; x++) {
        place_bold(player_ptr, 0, x, GB_EXTRA);
        place_bold(player_ptr, floor.height - 1, x, GB_EXTRA);
    }

    for (POSITION y = 1; y < (floor.height - 1); y++) {
        place_bold(player_ptr, y, 0, GB_EXTRA);
        place_bold(player_ptr, y, floor.width - 1, GB_EXTRA);
    }
}

static void place_cave_contents(PlayerType *player_ptr, DungeonData *dd_ptr, const DungeonDefinition &dungeon)
{
    auto &floor = *player_ptr->current_floor_ptr;
    if (floor.dun_level == 1) {
        constexpr auto density_moss = 2;
        while (one_in_(density_moss)) {
            const auto tmp_y = randint1(floor.height - 2);
            const auto tmp_x = randint1(floor.width - 2);
            place_trees(player_ptr, { tmp_y, tmp_x }); //!< @details 乱数引数の評価順を固定する.
        }
    }

    if (dd_ptr->destroyed) {
        destroy_level(player_ptr);
    }

    if (dungeon.has_river_flag() && one_in_(3) && (randint1(floor.dun_level) > 5)) {
        add_river(floor, dd_ptr);
    }

    // 追加でさらに川を増やす。
    if (one_in_(4)) {
        while (!one_in_(3)) {
            add_river(floor, dd_ptr);
        }
    }

    for (size_t i = 0; i < dd_ptr->cent_n; i++) {
        const auto pick = rand_range(0, i);
        std::swap(dd_ptr->centers[i], dd_ptr->centers[pick]);
    }
}

static bool decide_tunnel_planned_site(PlayerType *player_ptr, DungeonData *dd_ptr, const DungeonDefinition &dungeon, dt_type *dt_ptr, int i)
{
    dd_ptr->tunn_n = 0;
    dd_ptr->wall_n = 0;
    if (randint1(player_ptr->current_floor_ptr->dun_level) > dungeon.tunnel_percent) {
        (void)build_tunnel2(player_ptr, dd_ptr, dd_ptr->centers[i], dd_ptr->tunnel_pos, 2, 2);
    } else if (!build_tunnel(player_ptr, dd_ptr, dt_ptr, dd_ptr->centers[i], dd_ptr->tunnel_pos)) {
        dd_ptr->tunnel_fail_count++;
    }
    if (dd_ptr->tunnel_fail_count >= 200) {
        dd_ptr->why = _("トンネル接続に失敗", "Failed to generate tunnels");
        return false;
    } else {
        msg_format_wizard(player_ptr, CHEAT_DUNGEON, _("トンネル失敗回数:%d ", "Failure Tunnels:%d "), dd_ptr->tunnel_fail_count);
    }

    return true;
}

static void make_tunnels(PlayerType *player_ptr, DungeonData *dd_ptr)
{
    for (size_t i = 0; i < dd_ptr->tunn_n; i++) {
        dd_ptr->tunnel_pos = dd_ptr->tunnels[i];
        auto &grid = player_ptr->current_floor_ptr->get_grid(dd_ptr->tunnel_pos);
        const auto &terrain = grid.get_terrain();
        if (terrain.flags.has_not(TerrainCharacteristics::MOVE) || terrain.flags.has_none_of({ TerrainCharacteristics::WATER, TerrainCharacteristics::LAVA })) {
            grid.mimic = 0;
            place_grid(player_ptr, grid, GB_FLOOR);
        }
    }
}

static void make_walls(PlayerType *player_ptr, DungeonData *dd_ptr, const DungeonDefinition &dungeon, dt_type *dt_ptr)
{
    for (size_t j = 0; j < dd_ptr->wall_n; j++) {
        dd_ptr->tunnel_pos = dd_ptr->walls[j];
        auto &grid = player_ptr->current_floor_ptr->get_grid(dd_ptr->tunnel_pos);
        grid.mimic = 0;
        place_grid(player_ptr, grid, GB_FLOOR);
        if (evaluate_percent(dt_ptr->dun_tun_pen) && dungeon.flags.has_not(DungeonFeatureType::NO_DOORS)) {
            place_random_door(player_ptr, dd_ptr->tunnel_pos, true);
        }
    }
}

static bool make_centers(PlayerType *player_ptr, DungeonData *dd_ptr, const DungeonDefinition &dungeon, dt_type *dt_ptr)
{
    dd_ptr->tunnel_fail_count = 0;
    dd_ptr->door_n = 0;
    dd_ptr->tunnel_pos = dd_ptr->centers[dd_ptr->cent_n - 1];
    for (size_t i = 0; i < dd_ptr->cent_n; i++) {
        if (!decide_tunnel_planned_site(player_ptr, dd_ptr, dungeon, dt_ptr, i)) {
            return false;
        }

        make_tunnels(player_ptr, dd_ptr);
        make_walls(player_ptr, dd_ptr, dungeon, dt_ptr);
        dd_ptr->tunnel_pos = dd_ptr->centers[i];
    }

    return true;
}

static void make_doors(PlayerType *player_ptr, DungeonData *dd_ptr, dt_type *dt_ptr)
{
    for (size_t i = 0; i < dd_ptr->door_n; i++) {
        dd_ptr->tunnel_pos = dd_ptr->doors[i];
        for (const auto &d : Direction::directions_4()) {
            try_door(player_ptr, dt_ptr, dd_ptr->tunnel_pos + d.vec());
        }
    }
}

static void make_only_tunnel_points(const FloorType &floor, DungeonData *dd_ptr)
{
    int point_num = (floor.width * floor.height) / 200 + randint1(3);
    dd_ptr->cent_n = point_num;
    for (int i = 0; i < point_num; i++) {
        dd_ptr->centers[i].y = randint0(floor.height);
        dd_ptr->centers[i].x = randint0(floor.width);
    }
}

static bool make_one_floor(PlayerType *player_ptr, DungeonData *dd_ptr, const DungeonDefinition &dungeon)
{
    auto &floor = *player_ptr->current_floor_ptr;
    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::NO_ROOM)) {
        make_only_tunnel_points(floor, dd_ptr);
    } else {
        if (!generate_rooms(player_ptr, dd_ptr)) {
            dd_ptr->why = _("部屋群の生成に失敗", "Failed to generate rooms");
            return false;
        }
    }

    place_cave_contents(player_ptr, dd_ptr, dungeon);
    dt_type tmp_dt;
    dt_type *dt_ptr = initialize_dt_type(&tmp_dt);
    if (!make_centers(player_ptr, dd_ptr, dungeon, dt_ptr)) {
        return false;
    }

    dt_ptr = initialize_dt_type(&tmp_dt);
    if (!make_centers(player_ptr, dd_ptr, dungeon, dt_ptr)) {
        return false;
    }

    // 通路の過剰生成処理
    if (one_in_(2)) {
        int num = randint1(100);
        for (int i = 0; i < num; i++) {
            dt_ptr = initialize_dt_type(&tmp_dt);
            if (!make_centers(player_ptr, dd_ptr, dungeon, dt_ptr)) {
                return false;
            }
        }
    }

    make_doors(player_ptr, dd_ptr, dt_ptr);
    const auto &terrains = TerrainList::get_instance();
    if (!alloc_stairs(player_ptr, terrains.get_terrain_id(TerrainTag::DOWN_STAIR), Dice::roll(std::max(floor.width * floor.height / 4000, 1), 3), 3)) {
        dd_ptr->why = _("下り階段生成に失敗", "Failed to generate down stairs.");
        return false;
    }

    if (!alloc_stairs(player_ptr, terrains.get_terrain_id(TerrainTag::UP_STAIR), Dice::roll(std::max(floor.width * floor.height / 4000, 1), 3), 3)) {
        dd_ptr->why = _("上り階段生成に失敗", "Failed to generate up stairs.");
        return false;
    }

    return true;
}

static bool switch_making_floor(PlayerType *player_ptr, DungeonData *dd_ptr, const DungeonDefinition &dungeon)
{
    if (dungeon.flags.has(DungeonFeatureType::MAZE)) {
        const auto &floor = *player_ptr->current_floor_ptr;
        build_maze_vault(player_ptr, { floor.height / 2 - 1, floor.width / 2 - 1 }, { floor.height - 4, floor.width - 4 }, false);
        const auto &terrains = TerrainList::get_instance();
        if (!alloc_stairs(player_ptr, terrains.get_terrain_id(TerrainTag::DOWN_STAIR), rand_range(2, 3), 3)) {
            dd_ptr->why = _("迷宮ダンジョンの下り階段生成に失敗", "Failed to alloc up stairs in maze dungeon.");
            return false;
        }

        if (!alloc_stairs(player_ptr, terrains.get_terrain_id(TerrainTag::UP_STAIR), 1, 3)) {
            dd_ptr->why = _("迷宮ダンジョンの上り階段生成に失敗", "Failed to alloc down stairs in maze dungeon.");
            return false;
        }

        return true;
    }

    if (!make_one_floor(player_ptr, dd_ptr, dungeon)) {
        return false;
    }

    return true;
}

static void make_aqua_streams(PlayerType *player_ptr, DungeonData *dd_ptr, const DungeonDefinition &dungeon)
{
    if (dd_ptr->laketype != 0) {
        return;
    }

    if (dungeon.stream2 > 0) {
        constexpr auto num_quartz = 4;
        constexpr auto chance_quartz = 15;
        for (auto i = 0; i < num_quartz; i++) {
            build_streamer(player_ptr, dungeon.stream2, chance_quartz);
        }
    }

    if (dungeon.stream1 > 0) {
        constexpr auto num_magma = 6;
        constexpr auto chance_magma = 30;
        for (auto i = 0; i < num_magma; i++) {
            build_streamer(player_ptr, dungeon.stream1, chance_magma);
        }
    }
}

/*!
 * @brief マスにフロア端用の永久壁を配置する / Set boundary mimic and add "solid" perma-wall
 * @param grid 永久壁を配置したいグリッドへの参照
 */
static void place_bound_perm_wall(PlayerType *player_ptr, Grid &grid)
{
    if (bound_walls_perm) {
        grid.mimic = 0;
        place_grid(player_ptr, grid, GB_SOLID_PERM);
        return;
    }

    const auto &terrain = grid.get_terrain();
    if (terrain.flags.has_any_of({ TerrainCharacteristics::HAS_GOLD, TerrainCharacteristics::HAS_ITEM }) && terrain.flags.has_not(TerrainCharacteristics::SECRET)) {
        grid.feat = player_ptr->current_floor_ptr->get_dungeon_definition().convert_terrain_id(grid.feat, TerrainCharacteristics::ENSECRET);
    }

    grid.mimic = grid.feat;
    place_grid(player_ptr, grid, GB_SOLID_PERM);
}

static void make_perm_walls(PlayerType *player_ptr)
{
    auto &floor = *player_ptr->current_floor_ptr;
    for (POSITION x = 0; x < floor.width; x++) {
        place_bound_perm_wall(player_ptr, floor.get_grid({ 0, x }));
        place_bound_perm_wall(player_ptr, floor.get_grid({ floor.height - 1, x }));
    }

    for (POSITION y = 1; y < (floor.height - 1); y++) {
        place_bound_perm_wall(player_ptr, floor.get_grid({ y, 0 }));
        place_bound_perm_wall(player_ptr, floor.get_grid({ y, floor.width - 1 }));
    }
}

static bool check_place_necessary_objects(PlayerType *player_ptr, DungeonData *dd_ptr)
{
    const auto p_pos = new_player_spot(player_ptr);
    if (!p_pos) {
        dd_ptr->why = _("プレイヤー配置に失敗", "Failed to place a player");
        return false;
    }

    player_ptr->set_position(*p_pos);
    if (!place_quest_monsters(player_ptr)) {
        dd_ptr->why = _("クエストモンスター配置に失敗", "Failed to place a quest monster");
        return false;
    }

    return true;
}

static void decide_dungeon_data_allocation(PlayerType *player_ptr, DungeonData *dd_ptr, const DungeonDefinition &dungeon)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    dd_ptr->alloc_object_num = floor.dun_level / 3 + 5;
    if (dd_ptr->alloc_object_num > 30) {
        dd_ptr->alloc_object_num = 30;
    }

    if (one_in_(12)) {
        dd_ptr->alloc_object_num += randint1(50);
    }

    dd_ptr->alloc_monster_num = dungeon.min_m_alloc_level * dungeon.monster_rate / 100;
    if (floor.height >= MAX_HGT && floor.width >= MAX_WID) {
        return;
    }

    int small_tester = dd_ptr->alloc_monster_num * dungeon.monster_rate / 100;
    dd_ptr->alloc_monster_num = (dd_ptr->alloc_monster_num * floor.height) / MAX_HGT;
    dd_ptr->alloc_monster_num = (dd_ptr->alloc_monster_num * floor.width) / MAX_WID;
    dd_ptr->alloc_monster_num *= DUNGEON_MONSTER_MULTIPLE;
    dd_ptr->alloc_monster_num += 1;
    if (dd_ptr->alloc_monster_num > small_tester) {
        dd_ptr->alloc_monster_num = small_tester;
    } else {
        msg_format_wizard(player_ptr, CHEAT_DUNGEON, _("モンスター数基本値を %d から %d に減らします", "Reduced monsters base from %d to %d"), small_tester,
            dd_ptr->alloc_monster_num);
    }
}

static bool allocate_dungeon_data(PlayerType *player_ptr, DungeonData *dd_ptr, const DungeonDefinition &dungeon)
{
    dd_ptr->alloc_monster_num += randint1(8);
    for (dd_ptr->alloc_monster_num = dd_ptr->alloc_monster_num + dd_ptr->alloc_object_num; dd_ptr->alloc_monster_num > 0; dd_ptr->alloc_monster_num--) {
        (void)alloc_monster(player_ptr, 0, PM_ALLOW_SLEEP, summon_specific);
    }

    alloc_object(player_ptr, ALLOC_SET_BOTH, ALLOC_TYP_TRAP, randint1(dd_ptr->alloc_object_num * dungeon.trap_rate / 100));
    if (dungeon.flags.has_not(DungeonFeatureType::NO_CAVE)) {
        alloc_object(player_ptr, ALLOC_SET_CORR, ALLOC_TYP_RUBBLE, randint1(dd_ptr->alloc_object_num));
    }

    auto &floor = *player_ptr->current_floor_ptr;
    if (floor.is_entering_dungeon() && floor.dun_level > 1) {
        floor.object_level = 1;
    }

    constexpr auto alloc_room = 45;
    alloc_object(player_ptr, ALLOC_SET_ROOM, ALLOC_TYP_OBJECT, randnor(alloc_room, 3));
    constexpr auto alloc_item = 15;
    alloc_object(player_ptr, ALLOC_SET_BOTH, ALLOC_TYP_OBJECT, randnor(alloc_item, 3));
    constexpr auto alloc_gold = 15;
    alloc_object(player_ptr, ALLOC_SET_BOTH, ALLOC_TYP_GOLD, randnor(alloc_gold, 3));
    floor.object_level = floor.base_level;
    if (alloc_guardian(player_ptr, true)) {
        return true;
    }

    dd_ptr->why = _("ダンジョンの主配置に失敗", "Failed to place a dungeon guardian");
    return false;
}

static void decide_grid_glowing(FloorType &floor, DungeonData *dd_ptr, const DungeonDefinition &dungeon)
{
    constexpr auto chanle_wholly_dark = 5;
    auto is_empty_or_dark = dd_ptr->empty_level;
    is_empty_or_dark &= !one_in_(chanle_wholly_dark) || (randint1(100) > floor.dun_level);
    is_empty_or_dark &= dungeon.flags.has_not(DungeonFeatureType::DARKNESS);
    is_empty_or_dark &= dungeon.flags.has(DungeonFeatureType::ALWAY_LIGHT);
    if (!is_empty_or_dark) {
        return;
    }

    for (POSITION y = 0; y < floor.height; y++) {
        for (POSITION x = 0; x < floor.width; x++) {
            floor.grid_array[y][x].info |= CAVE_GLOW;
        }
    }
}

/*!
 * @brief ダンジョン生成のメインルーチン
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return ダンジョン生成が全て無事に成功したらnullopt、何かエラーがあったらその文字列
 */
std::optional<std::string> cave_gen(PlayerType *player_ptr)
{
    auto &floor = *player_ptr->current_floor_ptr;
    reset_lite_area(floor);
    get_mon_num_prep_enum(player_ptr, floor.get_monrace_hook());

    DungeonData dd({ floor.height, floor.width });
    auto &dungeon = floor.get_dungeon_definition();
    constexpr auto chance_empty_floor = 24;
    if (ironman_empty_levels || (dungeon.flags.has(DungeonFeatureType::ARENA) && (empty_levels && one_in_(chance_empty_floor)))) {
        dd.empty_level = true;
        msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("アリーナレベルを生成。", "Arena level."));
    }

    check_arena_floor(player_ptr, &dd);
    gen_caverns_and_lakes(player_ptr, dungeon, &dd);
    if (!switch_making_floor(player_ptr, &dd, dungeon)) {
        return dd.why;
    }

    make_aqua_streams(player_ptr, &dd, dungeon);
    make_perm_walls(player_ptr);
    if (!check_place_necessary_objects(player_ptr, &dd)) {
        return dd.why;
    }

    decide_dungeon_data_allocation(player_ptr, &dd, dungeon);
    if (!allocate_dungeon_data(player_ptr, &dd, dungeon)) {
        return dd.why;
    }

    decide_grid_glowing(floor, &dd, dungeon);
    return std::nullopt;
}
