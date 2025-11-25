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
#include "system/angband-system.h"
#include "system/dungeon/dungeon-data-definition.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "util/bit-flags-calculator.h"
#include "util/probability-table.h"
#include "util/rng-xoshiro.h"
#include "view/display-messages.h"
#include "wizard/wizard-messages.h"
#include "world/world-collapsion.h"
#include <vector>

// シンメトリックなフロア生成（左右対称）
static void make_symmetric_floor(FloorType &floor)
{
    // 左右対称化: 左半分を右半分にコピー
    for (int y = 0; y < floor.height; ++y) {
        for (int x = 0; x < floor.width / 2; ++x) {
            auto &left = floor.get_grid({ y, x });
            auto &right = floor.get_grid({ y, floor.width - 1 - x });
            right = left;
        }
    }
}

/*!
 * @brief BEGINNER持ち以外のダンジョンからランダムに1つを選択する
 * @return 選択されたダンジョンID、適切なダンジョンがない場合はnullopt
 */
static tl::optional<DungeonId> select_random_non_beginner_dungeon()
{
    std::vector<DungeonId> non_beginner_dungeons;
    const auto &dungeon_list = DungeonList::get_instance();

    // 全ダンジョンをチェックしてBEGINNER持ち以外を収集
    for (auto dungeon_id = i2enum<DungeonId>(1); enum2i(dungeon_id) <= DUNGEON_MAX; dungeon_id = i2enum<DungeonId>(enum2i(dungeon_id) + 1)) {
        const auto &dungeon = dungeon_list.get_dungeon(dungeon_id);
        if (dungeon.flags.has_not(DungeonFeatureType::BEGINNER)) {
            non_beginner_dungeons.push_back(dungeon_id);
        }
    }

    if (non_beginner_dungeons.empty()) {
        return tl::nullopt;
    }

    // ランダムに1つ選択
    const auto selected_index = randint0(non_beginner_dungeons.size());
    return non_beginner_dungeons[selected_index];
}

static void check_arena_floor(PlayerType *player_ptr, DungeonData *dd_ptr)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    if (!dd_ptr->empty_level) {
        for (const auto &pos : floor.get_area()) {
            place_bold(player_ptr, pos.y, pos.x, GB_EXTRA);
        }

        return;
    }

    for (const auto &pos : floor.get_area()) {
        place_bold(player_ptr, pos.y, pos.x, GB_FLOOR);
    }

    floor.get_area().each_edge([&](const Pos2D &pos) {
        place_bold(player_ptr, pos.y, pos.x, GB_EXTRA);
    });
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

    const auto always_river = dungeon.flags.has(DungeonFeatureType::ALWAYS_RIVER);
    if (always_river || (dungeon.has_river_flag() && one_in_(3) && (randint1(floor.dun_level) > 5))) {
        add_river(floor, dd_ptr);
    }

    // 追加でさらに川を増やす。
    if (always_river || one_in_(4)) {
        while (!one_in_(3)) {
            add_river(floor, dd_ptr);
        }
    }

    for (size_t i = 0; i < dd_ptr->cent_n; i++) {
        const auto pick = rand_range(0, i);
        std::swap(dd_ptr->centers[i], dd_ptr->centers[pick]);
    }
}

/*!
 * @brief 環状水路を生成する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details マップの外周に沿ってマンハッタン距離で水地形の環状路を生成する
 */
static void generate_circular_waterway(PlayerType *player_ptr)
{
    auto &floor = *player_ptr->current_floor_ptr;
    const auto margin = 2; // 外壁からのマージン

    // 環状水路の座標を計算
    const auto start_y = margin;
    const auto end_y = floor.height - margin - 1;
    const auto start_x = margin;
    const auto end_x = floor.width - margin - 1;

    // 上辺: 左から右へ
    for (int x = start_x; x <= end_x; x++) {
        place_bold(player_ptr, start_y, x, GB_WATER);
    }

    // 右辺: 上から下へ（角を重複させないため start_y+1 から）
    for (int y = start_y + 1; y <= end_y; y++) {
        place_bold(player_ptr, y, end_x, GB_WATER);
    }

    // 下辺: 右から左へ（角を重複させないため end_x-1 から）
    for (int x = end_x - 1; x >= start_x; x--) {
        place_bold(player_ptr, end_y, x, GB_WATER);
    }

    // 左辺: 下から上へ（角を重複させないため end_y-1 から start_y+1 まで）
    for (int y = end_y - 1; y > start_y; y--) {
        place_bold(player_ptr, y, start_x, GB_WATER);
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
    if (dungeon.flags.has(DungeonFeatureType::NO_ROOM)) {
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
    if (one_in_(8)) {
        int num = randint1((floor.width * floor.height) / 500);
        if (one_in_(2)) {
            num /= 2;
        }
        if (one_in_(2)) {
            num /= 3;
        }
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

    // ポータル配置処理（1/3の確率で1～2個配置）
    if (one_in_(3)) {
        const auto portal_num = randint1(2);
        if (!alloc_stairs(player_ptr, terrains.get_terrain_id(TerrainTag::PORTAL), portal_num, 3)) {
            // ポータル配置失敗は警告のみで処理継続
            msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("ポータル生成に失敗", "Failed to generate portals."));
        }
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

        // 迷宮ダンジョンでもポータル配置処理（1/3の確率で1～2個配置）
        if (one_in_(3)) {
            const auto portal_num = randint1(2);
            if (!alloc_stairs(player_ptr, terrains.get_terrain_id(TerrainTag::PORTAL), portal_num, 3)) {
                // ポータル配置失敗は警告のみで処理継続
                msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("迷宮ダンジョンのポータル生成に失敗", "Failed to generate portals in maze dungeon."));
            }
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
    floor.get_area().each_edge([&](const Pos2D &pos) {
        place_bound_perm_wall(player_ptr, floor.get_grid(pos));
    });
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

    // 特定階層でのダイスベースアイテム生成
    alloc_specific_floor_items(player_ptr);

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
    is_empty_or_dark = dungeon.flags.has(DungeonFeatureType::ALWAYS_LIGHT);
    if (!is_empty_or_dark) {
        return;
    }

    for (const auto &pos : floor.get_area()) {
        floor.get_grid(pos).add_info(CAVE_GLOW);
    }
}

/*!
 * @brief ダンジョン生成のメインルーチン
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param seed 乱数の種（オプショナル）。指定された場合は固定ダンジョンを生成
 * @return ダンジョン生成が全て無事に成功したらnullopt、何かエラーがあったらその文字列
 */
tl::optional<std::string> cave_gen(PlayerType *player_ptr, tl::optional<uint32_t> seed)
{
    // 乱数種の保存と復元用
    Xoshiro128StarStar::state_type original_state{};
    bool seed_was_fixed = false;

    // 乱数種が指定された場合は固定
    if (seed) {
        auto &rng = AngbandSystem::get_instance().get_rng();
        original_state = rng.get_state(); // 現在の乱数状態を保存
        rng = Xoshiro128StarStar(*seed); // 指定された種で乱数を初期化
        seed_was_fixed = true;
        msg_format_wizard(player_ptr, CHEAT_DUNGEON,
            _("乱数種を固定してダンジョンを生成: 0x%08X", "Generating dungeon with fixed seed: 0x%08X"),
            *seed);
    }

    auto &floor = *player_ptr->current_floor_ptr;
    floor.reset_lite_area();
    get_mon_num_prep_enum(player_ptr, floor.get_monrace_hook());

    // 鉄獄（ANGBAND）では一定確率で他のダンジョンの生成処理を使用
    constexpr auto chance_random_dungeon = 10; // 10%の確率
    if (floor.dungeon_id == DungeonId::ANGBAND && one_in_(chance_random_dungeon)) {
        if (auto random_dungeon_id = select_random_non_beginner_dungeon()) {
            floor.dungeon_generated_id = *random_dungeon_id;
            msg_print_wizard(player_ptr, CHEAT_DUNGEON,
                format(_("鉄獄で他ダンジョンの生成処理を使用: %s", "Using random dungeon generation in Angband: %s"),
                    floor.get_generated_dungeon_definition().name.data()));
        } else {
            floor.dungeon_generated_id = floor.dungeon_id;
        }
    } else {
        floor.dungeon_generated_id = floor.dungeon_id;
    }

    DungeonData dd({ floor.height, floor.width });
    auto &dungeon = floor.get_generated_dungeon_definition();
    floor.allianceID = dungeon.alliance_idx;

    constexpr auto chance_empty_floor = 24;
    if (ironman_empty_levels || (dungeon.flags.has(DungeonFeatureType::ARENA) && (empty_levels && one_in_(chance_empty_floor)))) {
        dd.empty_level = true;
        msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("アリーナレベルを生成。", "Arena level."));
    }

    // ALWAY_ARENAフラグがある場合は常にアリーナ地形にする
    if (dungeon.flags.has(DungeonFeatureType::ALWAY_ARENA)) {
        dd.empty_level = true;
        msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("常時アリーナレベルを生成。", "Always arena level."));
    }

    check_arena_floor(player_ptr, &dd);
    gen_caverns_and_lakes(player_ptr, dungeon, &dd);
    if (!switch_making_floor(player_ptr, &dd, dungeon)) {
        // 乱数状態を復元
        if (seed_was_fixed) {
            AngbandSystem::get_instance().get_rng().set_state(original_state);
        }
        return dd.why;
    }

    make_aqua_streams(player_ptr, &dd, dungeon);
    make_perm_walls(player_ptr);

    // 環状水路の生成（WATERWAYフラグを持つダンジョンで1/8の確率）
    constexpr int waterway_chance = 8;
    if (dungeon.flags.has(DungeonFeatureType::WATERWAY) && one_in_(waterway_chance) && !dd.empty_level) {
        generate_circular_waterway(player_ptr);
        msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("環状水路を生成。", "Circular waterway generated."));
    }

    // 地形生成完了後、モンスター・アイテム配置前に左右対称化
    constexpr int symmetric_chance = 25;
    if (one_in_(symmetric_chance)) {
        make_symmetric_floor(floor);
        msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("シンメトリックなフロアを生成。", "Symmetric floor generated."));
    }

    if (!check_place_necessary_objects(player_ptr, &dd)) {
        // 乱数状態を復元
        if (seed_was_fixed) {
            AngbandSystem::get_instance().get_rng().set_state(original_state);
        }
        return dd.why;
    }

    decide_dungeon_data_allocation(player_ptr, &dd, dungeon);
    if (!allocate_dungeon_data(player_ptr, &dd, dungeon)) {
        // 乱数状態を復元
        if (seed_was_fixed) {
            AngbandSystem::get_instance().get_rng().set_state(original_state);
        }
        return dd.why;
    }

    decide_grid_glowing(floor, &dd, dungeon);

    // VESTIGEフラグを持つダンジョンで地形をランダムに差し替える
    if (dungeon.flags.has(DungeonFeatureType::VESTIGE)) {
        apply_vestige_terrain_replacement(player_ptr);
    }

    // 時空崩壊度に応じて虚空地形を配置
    apply_void_terrain_placement(player_ptr);

    // 乱数状態を復元
    if (seed_was_fixed) {
        AngbandSystem::get_instance().get_rng().set_state(original_state);
        msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("乱数状態を復元", "Random state restored"));
    }

    return tl::nullopt;
}

/*!
 * @brief VESTIGEフラグを持つダンジョンで地形をランダムに差し替える
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details フロア全体を走査し、4%の確率でPERMANENTフラグを持たない地形をランダムに差し替える
 */
void apply_vestige_terrain_replacement(PlayerType *player_ptr)
{
    auto &floor = *player_ptr->current_floor_ptr;
    auto &terrains = TerrainList::get_instance();

    // PERMANENTフラグを持たない地形IDのリストを作成
    std::vector<int> replaceable_terrain_ids;
    for (const auto &terrain : terrains) {
        if (terrain.flags.has_not(TerrainCharacteristics::PERMANENT)) {
            replaceable_terrain_ids.push_back(terrain.idx);
        }
    }

    if (replaceable_terrain_ids.empty()) {
        return; // 差し替え可能な地形がない場合は何もしない
    }

    // フロア全体を走査して地形を差し替え
    constexpr int replacement_chance = 25; // 4% = 1/25
    int replacement_count = 0;

    for (const auto &pos : floor.get_area()) {
        auto &grid = floor.get_grid(pos);
        const auto &current_terrain = terrains.get_terrain(grid.feat);

        // PERMANENTフラグを持つ地形はスキップ
        if (current_terrain.flags.has(TerrainCharacteristics::PERMANENT)) {
            continue;
        }

        // 4%の確率で地形を差し替え
        if (one_in_(replacement_chance)) {
            const auto terrain = rand_choice(replaceable_terrain_ids);
            set_terrain_id_to_grid(player_ptr, pos, terrain);
            replacement_count++;
        }
    }

    msg_print_wizard(player_ptr, CHEAT_DUNGEON,
        format(_("VESTIGE効果: %d箇所の地形を差し替えました", "VESTIGE effect: Replaced %d terrain features"),
            replacement_count));
}

/*!
 * @brief 時空崩壊度に応じて虚空地形を配置する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details 時空崩壊度が70%を超えると、線形的に増加する確率で地形を虚空に置き換える
 */
void apply_void_terrain_placement(PlayerType *player_ptr)
{
    // 時空崩壊度が70%（70,000,000百万分率）を超えているかチェック
    const int32_t collapse_threshold = 70000000; // 70%
    if (wc_ptr->collapse_degree <= collapse_threshold) {
        return;
    }

    auto &floor = *player_ptr->current_floor_ptr;
    auto &terrains = TerrainList::get_instance();

    // 時空崩壊度から配置率を計算
    // 70%時点で3%、100%時点で30%まで線形的に増加
    const int32_t excess_collapse = wc_ptr->collapse_degree - collapse_threshold;
    const int32_t max_excess = 100000000 - collapse_threshold; // 30%分の範囲

    // 配置率を計算（3%～30%の範囲で線形補間）
    int placement_rate_percent = 3 + (27 * excess_collapse / max_excess); // 3%～30%

    // 上限チェック
    if (placement_rate_percent > 30) {
        placement_rate_percent = 30;
    }

    // ゼロ除算回避
    if (placement_rate_percent <= 0) {
        placement_rate_percent = 1;
    }

    const int placement_chance = 100 / placement_rate_percent; // 確率の逆数

    // 虚空地形のID (VOID_ZONE = 236)
    constexpr int void_terrain_id = 236;

    int placement_count = 0;

    // フロア全体を走査して地形を虚空に置き換え
    for (const auto &pos : floor.get_area()) {
        auto &grid = floor.get_grid(pos);
        const auto &current_terrain = terrains.get_terrain(grid.feat);

        // PERMANENTフラグを持つ地形はスキップ
        if (current_terrain.flags.has(TerrainCharacteristics::PERMANENT)) {
            continue;
        }

        // 計算された確率で地形を虚空に置き換え
        if (one_in_(placement_chance)) {
            set_terrain_id_to_grid(player_ptr, pos, void_terrain_id);
            placement_count++;
        }
    }

    msg_print_wizard(player_ptr, CHEAT_DUNGEON,
        format(_("時空崩壊効果: %d箇所を虚空地形に置き換えました（崩壊度: %d.%06d%%、配置率: %d%%）",
                   "World collapse effect: Replaced %d terrain features with void (collapse: %d.%06d%%, rate: %d%%)"),
            placement_count,
            wc_ptr->collapse_degree / 1000000, wc_ptr->collapse_degree % 1000000,
            placement_rate_percent));
}
