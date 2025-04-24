/*!
 * @brief 荒野マップの生成とルール管理実装
 * @date 2025/02/01
 * @author
 * Robert A. Koeneke, 1983
 * James E. Wilson, 1989
 * Deskull, 2013
 * Hourier, 2025
 */

#include "floor/wild.h"
#include "core/asking-player.h"
#include "dungeon/quest.h"
#include "game-option/birth-options.h"
#include "game-option/map-screen-options.h"
#include "info-reader/fixed-map-parser.h"
#include "info-reader/parse-error-types.h"
#include "io/tokenizer.h"
#include "market/building-initializer.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/monster-remover.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-util.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/player-status.h"
#include "room/rooms-vault.h"
#include "spell-realm/spells-hex.h"
#include "status/action-setter.h"
#include "system/angband-system.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/enums/terrain/wilderness-terrain.h"
#include "system/floor/floor-info.h"
#include "system/floor/town-info.h"
#include "system/floor/town-list.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "window/main-window-util.h"
#include "world/world.h"
#include <map>
#include <numeric>
#include <utility>

constexpr auto SUM_TERRAIN_PROBABILITIES = 18;

std::vector<std::vector<WildernessGrid>> wilderness;

bool reinit_wilderness = false;

static bool generate_encounter;

struct border_type {
    short top[MAX_WID];
    short bottom[MAX_WID];
    short right[MAX_HGT];
    short left[MAX_HGT];
    short top_left;
    short top_right;
    short bottom_left;
    short bottom_right;
};

static border_type border;

static std::vector<WildernessGrid> wilderness_letters;

/* The default table in terrain level generation. */
static std::map<WildernessTerrain, std::map<short, TerrainTag>> terrain_table;

static const std::map<WildernessTerrain, TerrainTag> WT_TT_MAP = {
    { WildernessTerrain::EDGE, TerrainTag::PERMANENT_WALL },
    { WildernessTerrain::TOWN, TerrainTag::TOWN },
    { WildernessTerrain::DEEP_WATER, TerrainTag::DEEP_WATER },
    { WildernessTerrain::SHALLOW_WATER, TerrainTag::SHALLOW_WATER },
    { WildernessTerrain::SWAMP, TerrainTag::SWAMP },
    { WildernessTerrain::DIRT, TerrainTag::DIRT },
    { WildernessTerrain::GRASS, TerrainTag::GRASS },
    { WildernessTerrain::TREES, TerrainTag::TREE },
    { WildernessTerrain::DESERT, TerrainTag::DIRT },
    { WildernessTerrain::SHALLOW_LAVA, TerrainTag::SHALLOW_LAVA },
    { WildernessTerrain::DEEP_LAVA, TerrainTag::DEEP_LAVA },
    { WildernessTerrain::MOUNTAIN, TerrainTag::MOUNTAIN },
};

/*!
 * @brief プラズマフラクタル的地形生成の再帰中間処理
 * / Helper for plasma generation.
 * @param x1 左上端の深み
 * @param x2 右上端の深み
 * @param x3 左下端の深み
 * @param x4 右下端の深み
 * @param xmid 中央座標X
 * @param ymid 中央座標Y
 * @param rough ランダム幅
 * @param depth_max 深みの最大値
 */
static void perturb_point_mid(
    FloorType &floor, FEAT_IDX x1, FEAT_IDX x2, FEAT_IDX x3, FEAT_IDX x4, POSITION xmid, POSITION ymid, FEAT_IDX rough, FEAT_IDX depth_max)
{
    FEAT_IDX tmp2 = rough * 2 + 1;
    FEAT_IDX tmp = randint1(tmp2) - (rough + 1);
    FEAT_IDX avg = ((x1 + x2 + x3 + x4) / 4) + tmp;
    if (((x1 + x2 + x3 + x4) % 4) > 1) {
        avg++;
    }

    if (avg < 0) {
        avg = 0;
    }

    if (avg > depth_max) {
        avg = depth_max;
    }

    floor.get_grid({ ymid, xmid }).feat = avg;
}

/*!
 * @brief プラズマフラクタル的地形生成の再帰末端処理
 * / Helper for plasma generation.
 * @param x1 中間末端部1の重み
 * @param x2 中間末端部2の重み
 * @param x3 中間末端部3の重み
 * @param xmid 最終末端部座標X
 * @param ymid 最終末端部座標Y
 * @param rough ランダム幅
 * @param depth_max 深みの最大値
 */
static void perturb_point_end(FloorType &floor, FEAT_IDX x1, FEAT_IDX x2, FEAT_IDX x3, POSITION xmid, POSITION ymid, FEAT_IDX rough, FEAT_IDX depth_max)
{
    FEAT_IDX tmp2 = rough * 2 + 1;
    FEAT_IDX tmp = randint0(tmp2) - rough;
    FEAT_IDX avg = ((x1 + x2 + x3) / 3) + tmp;
    if ((x1 + x2 + x3) % 3) {
        avg++;
    }

    if (avg < 0) {
        avg = 0;
    }

    if (avg > depth_max) {
        avg = depth_max;
    }

    floor.get_grid({ ymid, xmid }).feat = avg;
}

/*!
 * @brief プラズマフラクタル的地形生成の開始処理
 * / Helper for plasma generation.
 * @param x1 処理範囲の左上X座標
 * @param y1 処理範囲の左上Y座標
 * @param x2 処理範囲の右下X座標
 * @param y2 処理範囲の右下Y座標
 * @param depth_max 深みの最大値
 * @param rough ランダム幅
 * @details
 * <pre>
 * A generic function to generate the plasma fractal.
 * Note that it uses ``cave_feat'' as temporary storage.
 * The values in ``cave_feat'' after this function
 * are NOT actual features; They are raw heights which
 * need to be converted to features.
 * </pre>
 */
static void plasma_recursive(FloorType &floor, POSITION x1, POSITION y1, POSITION x2, POSITION y2, FEAT_IDX depth_max, FEAT_IDX rough)
{
    POSITION xmid = (x2 - x1) / 2 + x1;
    POSITION ymid = (y2 - y1) / 2 + y1;
    if (x1 + 1 == x2) {
        return;
    }

    perturb_point_mid(floor, floor.get_grid({ y1, x1 }).feat, floor.get_grid({ y2, x1 }).feat, floor.get_grid({ y1, x2 }).feat,
        floor.get_grid({ y2, x2 }).feat, xmid, ymid, rough, depth_max);
    perturb_point_end(
        floor, floor.get_grid({ y1, x1 }).feat, floor.get_grid({ y1, x2 }).feat, floor.get_grid({ ymid, xmid }).feat, xmid, y1, rough, depth_max);
    perturb_point_end(
        floor, floor.get_grid({ y1, x2 }).feat, floor.get_grid({ y2, x2 }).feat, floor.get_grid({ ymid, xmid }).feat, x2, ymid, rough, depth_max);
    perturb_point_end(
        floor, floor.get_grid({ y2, x2 }).feat, floor.get_grid({ y2, x1 }).feat, floor.get_grid({ ymid, xmid }).feat, xmid, y2, rough, depth_max);
    perturb_point_end(
        floor, floor.get_grid({ y2, x1 }).feat, floor.get_grid({ y1, x1 }).feat, floor.get_grid({ ymid, xmid }).feat, x1, ymid, rough, depth_max);
    plasma_recursive(floor, x1, y1, xmid, ymid, depth_max, rough);
    plasma_recursive(floor, xmid, y1, x2, ymid, depth_max, rough);
    plasma_recursive(floor, x1, ymid, xmid, y2, depth_max, rough);
    plasma_recursive(floor, xmid, ymid, x2, y2, depth_max, rough);
}

/*!
 * @brief 荒野フロア生成のサブルーチン
 * @param terrain 荒野地形ID
 * @param seed 乱数の固定シード
 * @param border 未使用
 * @param corner 広域マップの角部分としての生成ならばTRUE
 */
static void generate_wilderness_area(FloorType &floor, WildernessTerrain terrain, uint32_t seed, bool corner)
{
    if (terrain == WildernessTerrain::EDGE) {
        for (auto y = 0; y < MAX_HGT; y++) {
            for (auto x = 0; x < MAX_WID; x++) {
                floor.get_grid({ y, x }).set_terrain_id(TerrainTag::PERMANENT_WALL);
            }
        }

        return;
    }

    auto &system = AngbandSystem::get_instance();
    const Xoshiro128StarStar rng_backup = system.get_rng();
    Xoshiro128StarStar wilderness_rng(seed);
    system.set_rng(wilderness_rng);
    if (!corner) {
        for (auto y = 0; y < MAX_HGT; y++) {
            for (auto x = 0; x < MAX_WID; x++) {
                floor.get_grid({ y, x }).feat = SUM_TERRAIN_PROBABILITIES / 2;
            }
        }
    }

    auto &grid_top_left = floor.get_grid({ 1, 1 });
    auto &grid_bottom_left = floor.get_grid({ MAX_HGT - 2, 1 });
    auto &grid_top_right = floor.get_grid({ 1, MAX_WID - 2 });
    auto &grid_bottom_right = floor.get_grid({ MAX_HGT - 2, MAX_WID - 2 });
    grid_top_left.feat = randnum0<short>(SUM_TERRAIN_PROBABILITIES);
    grid_bottom_left.feat = randnum0<short>(SUM_TERRAIN_PROBABILITIES);
    grid_top_right.feat = randnum0<short>(SUM_TERRAIN_PROBABILITIES);
    grid_bottom_right.feat = randnum0<short>(SUM_TERRAIN_PROBABILITIES);
    if (corner) {
        const auto &tags = terrain_table.at(terrain);
        grid_top_left.set_terrain_id(tags.at(grid_top_left.feat));
        grid_bottom_left.set_terrain_id(tags.at(grid_bottom_left.feat));
        grid_top_right.set_terrain_id(tags.at(grid_top_right.feat));
        grid_bottom_right.set_terrain_id(tags.at(grid_bottom_right.feat));
        system.set_rng(rng_backup);
        return;
    }

    const auto top_left = grid_top_left.feat;
    const auto bottom_left = grid_bottom_left.feat;
    const auto top_right = grid_top_right.feat;
    const auto bottom_right = grid_bottom_right.feat;
    const short roughness = 1; /* The roughness of the level. */
    plasma_recursive(floor, 1, 1, MAX_WID - 2, MAX_HGT - 2, SUM_TERRAIN_PROBABILITIES - 1, roughness);
    grid_top_left.feat = top_left;
    grid_bottom_left.feat = bottom_left;
    grid_top_right.feat = top_right;
    grid_bottom_right.feat = bottom_right;
    for (auto y = 1; y < MAX_HGT - 1; y++) {
        for (auto x = 1; x < MAX_WID - 1; x++) {
            const Pos2D pos(y, x);
            auto &grid = floor.get_grid(pos);
            grid.set_terrain_id(terrain_table.at(terrain).at(grid.feat));
        }
    }

    system.set_rng(rng_backup);
}

/*!
 * @brief 荒野フロア生成のメインルーチン
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 広域Y座標
 * @param x 広域X座標
 * @param is_border 広域マップの辺部分としての生成ならばTRUE
 * @param is_corner 広域マップの角部分としての生成ならばTRUE
 */
static void generate_area(PlayerType *player_ptr, const Pos2D &pos, bool is_border, bool is_corner)
{
    const auto &wilderness_grid = wilderness[pos.y][pos.x];
    player_ptr->town_num = wilderness_grid.town;
    auto &floor = *player_ptr->current_floor_ptr;
    floor.base_level = wilderness_grid.level;
    floor.dun_level = 0;
    floor.monster_level = floor.base_level;
    floor.object_level = floor.base_level;
    if (player_ptr->town_num) {
        init_buildings();
        if (is_border || is_corner) {
            init_flags = i2enum<init_flags_type>(INIT_CREATE_DUNGEON | INIT_ONLY_FEATURES);
        } else {
            init_flags = INIT_CREATE_DUNGEON;
        }

        floor.vault_list.clear();
        parse_fixed_map(player_ptr, TOWN_DEFINITION_LIST, 0, 0, MAX_HGT, MAX_WID);
        floor.width = MAX_WID;
        floor.height = MAX_HGT;

        for (auto tv : floor.vault_list) {
            const auto vault = &vaults_info[static_cast<int>(tv.id)];
            build_vault(player_ptr, tv.y, tv.x, vault->hgt, vault->wid, vault->text.data(), tv.yoffset, tv.xoffset, tv.transno);
        }

        if (!is_corner && !is_border) {
            player_ptr->visit |= (1UL << (player_ptr->town_num - 1));
        }
    } else {
        const auto terrain = wilderness_grid.terrain;
        const auto seed = wilderness_grid.seed;
        generate_wilderness_area(floor, terrain, seed, is_corner);
    }

    if (!is_corner && (wilderness_grid.town == 0)) {
        //!< @todo make the road a bit more interresting.
        if (wilderness_grid.road) {
            floor.get_grid({ MAX_HGT / 2, MAX_WID / 2 }).set_terrain_id(TerrainTag::FLOOR);
            if (wilderness[pos.y - 1][pos.x].road) {
                /* North road */
                for (auto y = 1; y < MAX_HGT / 2; y++) {
                    const Pos2D pos_road(y, MAX_WID / 2);
                    floor.get_grid(pos_road).set_terrain_id(TerrainTag::FLOOR);
                }
            }

            if (wilderness[pos.y + 1][pos.x].road) {
                /* North road */
                for (auto y = MAX_HGT / 2; y < MAX_HGT - 1; y++) {
                    const Pos2D pos_road(y, MAX_WID / 2);
                    floor.get_grid(pos_road).set_terrain_id(TerrainTag::FLOOR);
                }
            }

            if (wilderness[pos.y][pos.x + 1].road) {
                /* East road */
                for (auto x = MAX_WID / 2; x < MAX_WID - 1; x++) {
                    const Pos2D pos_road(MAX_HGT / 2, x);
                    floor.get_grid(pos_road).set_terrain_id(TerrainTag::FLOOR);
                }
            }

            if (wilderness[pos.y][pos.x - 1].road) {
                /* West road */
                for (auto x = 1; x < MAX_WID / 2; x++) {
                    const Pos2D pos_road(MAX_HGT / 2, x);
                    floor.get_grid(pos_road).set_terrain_id(TerrainTag::FLOOR);
                }
            }
        }
    }

    const auto entrance = wilderness_grid.entrance;
    auto is_winner = entrance > DungeonId::WILDERNESS;
    is_winner &= (wilderness_grid.town == 0);
    auto is_wild_winner = DungeonList::get_instance().get_dungeon(entrance).flags.has_not(DungeonFeatureType::WINNER);
    is_winner &= ((AngbandWorld::get_instance().total_winner != 0) || is_wild_winner);
    if (!is_winner) {
        return;
    }

    auto &system = AngbandSystem::get_instance();
    const Xoshiro128StarStar rng_backup = system.get_rng();
    Xoshiro128StarStar wilderness_rng(wilderness_grid.seed);
    system.set_rng(wilderness_rng);
    const Pos2D pos_entrance(rand_range(6, floor.height - 6), rand_range(6, floor.width - 6));
    floor.get_grid(pos_entrance).set_terrain_id(TerrainTag::ENTRANCE);
    floor.get_grid(pos_entrance).special = static_cast<short>(wilderness_grid.entrance);
    system.set_rng(rng_backup);
}

/*!
 * @brief 地上マップにモンスターを生成する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details '>' キーで普通に入った時と、襲撃を受けた時でモンスター数は異なる.
 * また、集団生成や護衛は、最初に生成された1体だけがカウント対象である.
 * よって、実際に生成されるモンスターは、コードの見た目より多くなる.
 */
static void generate_wild_monsters(PlayerType *player_ptr)
{
    constexpr auto num_ambush_monsters = 100;
    constexpr auto num_normal_monsters = 25;
    const auto lim = generate_encounter ? num_ambush_monsters : num_normal_monsters;
    for (auto i = 0; i < lim; i++) {
        BIT_FLAGS mode = 0;
        if (!(generate_encounter || (one_in_(2) && (!player_ptr->town_num)))) {
            mode |= PM_ALLOW_SLEEP;
        }

        (void)alloc_monster(player_ptr, generate_encounter ? 0 : 3, mode, summon_specific);
    }
}

/*!
 * @brief 広域マップの生成 /
 * Build the wilderness area outside of the town.
 * @todo 広域マップは恒常生成にする予定、PlayerTypeによる処理分岐は最終的に排除する。
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void wilderness_gen(PlayerType *player_ptr)
{
    auto &floor = *player_ptr->current_floor_ptr;
    floor.height = MAX_HGT;
    floor.width = MAX_WID;
    panel_row_min = floor.height;
    panel_col_min = floor.width;
    auto &world = AngbandWorld::get_instance();
    parse_fixed_map(player_ptr, WILDERNESS_DEFINITION, 0, 0, world.max_wild_y, world.max_wild_x);
    const auto wild_y = player_ptr->wilderness_y;
    const auto wild_x = player_ptr->wilderness_x;
    get_mon_num_prep_enum(player_ptr, get_monster_hook({ wild_y, wild_x }, floor.is_underground()));

    /* North border */
    generate_area(player_ptr, { wild_y - 1, wild_x }, true, false);
    for (auto i = 1; i < MAX_WID - 1; i++) {
        border.top[i] = floor.get_grid({ MAX_HGT - 2, i }).feat;
    }

    /* South border */
    generate_area(player_ptr, { wild_y + 1, wild_x }, true, false);
    for (auto i = 1; i < MAX_WID - 1; i++) {
        border.bottom[i] = floor.get_grid({ 1, i }).feat;
    }

    /* West border */
    generate_area(player_ptr, { wild_y, wild_x - 1 }, true, false);
    for (auto i = 1; i < MAX_HGT - 1; i++) {
        border.left[i] = floor.get_grid({ i, MAX_WID - 2 }).feat;
    }

    /* East border */
    generate_area(player_ptr, { wild_y, wild_x + 1 }, true, false);
    for (auto i = 1; i < MAX_HGT - 1; i++) {
        border.right[i] = floor.get_grid({ i, 1 }).feat;
    }

    /* North west corner */
    generate_area(player_ptr, { wild_y - 1, wild_x - 1 }, false, true);
    border.top_left = floor.get_grid({ MAX_HGT - 2, MAX_WID - 2 }).feat;

    /* North east corner */
    generate_area(player_ptr, { wild_y - 1, wild_x + 1 }, false, true);
    border.top_right = floor.get_grid({ MAX_HGT - 2, 1 }).feat;

    /* South west corner */
    generate_area(player_ptr, { wild_y + 1, wild_x - 1 }, false, true);
    border.bottom_left = floor.get_grid({ 1, MAX_WID - 2 }).feat;

    /* South east corner */
    generate_area(player_ptr, { wild_y + 1, wild_x + 1 }, false, true);
    border.bottom_right = floor.get_grid({ 1, 1 }).feat;

    /* Create terrain of the current area */
    generate_area(player_ptr, { wild_y, wild_x }, false, false);

    /* Special boundary walls -- North */
    for (auto i = 0; i < MAX_WID; i++) {
        auto &grid = floor.get_grid({ 0, i });
        grid.set_terrain_id(TerrainTag::PERMANENT_WALL);
        grid.mimic = border.top[i];
    }

    /* Special boundary walls -- South */
    for (auto i = 0; i < MAX_WID; i++) {
        auto &grid = floor.get_grid({ MAX_HGT - 1, i });
        grid.set_terrain_id(TerrainTag::PERMANENT_WALL);
        grid.mimic = border.bottom[i];
    }

    /* Special boundary walls -- West */
    for (auto i = 0; i < MAX_HGT; i++) {
        auto &grid = floor.get_grid({ i, 0 });
        grid.set_terrain_id(TerrainTag::PERMANENT_WALL);
        grid.mimic = border.left[i];
    }

    /* Special boundary walls -- East */
    for (auto i = 0; i < MAX_HGT; i++) {
        auto &grid = floor.get_grid({ i, MAX_WID - 1 });
        grid.set_terrain_id(TerrainTag::PERMANENT_WALL);
        grid.mimic = border.right[i];
    }

    floor.get_grid({ 0, 0 }).mimic = border.top_left;
    floor.get_grid({ 0, MAX_WID - 1 }).mimic = border.top_right;
    floor.get_grid({ MAX_HGT - 1, 0 }).mimic = border.bottom_left;
    floor.get_grid({ MAX_HGT - 1, MAX_WID - 1 }).mimic = border.bottom_right;
    for (auto y = 0; y < floor.height; y++) {
        for (auto x = 0; x < floor.width; x++) {
            auto &grid = floor.get_grid({ y, x });
            if (world.is_daytime()) {
                grid.info |= CAVE_GLOW;
                if (view_perma_grids) {
                    grid.info |= CAVE_MARK;
                }

                continue;
            }

            const auto &terrain = grid.get_terrain(TerrainKind::MIMIC);
            auto can_darken = !grid.is_mirror();
            can_darken &= terrain.flags.has_none_of({ TerrainCharacteristics::QUEST_ENTER, TerrainCharacteristics::ENTRANCE });
            if (can_darken) {
                grid.info &= ~(CAVE_GLOW);
                if (terrain.flags.has_not(TerrainCharacteristics::REMEMBER)) {
                    grid.info &= ~(CAVE_MARK);
                }

                continue;
            }

            if (terrain.flags.has_not(TerrainCharacteristics::ENTRANCE)) {
                continue;
            }

            grid.info |= CAVE_GLOW;
            if (view_perma_grids) {
                grid.info |= CAVE_MARK;
            }
        }
    }

    if (player_ptr->teleport_town) {
        for (auto y = 0; y < floor.height; y++) {
            for (auto x = 0; x < floor.width; x++) {
                auto &grid = floor.get_grid({ y, x });
                const auto &terrain = grid.get_terrain();
                if (terrain.flags.has_not(TerrainCharacteristics::BLDG)) {
                    continue;
                }

                if ((terrain.subtype != 4) && !((player_ptr->town_num == 1) && (terrain.subtype == 0))) {
                    continue;
                }

                if (grid.has_monster()) {
                    delete_monster_idx(player_ptr, grid.m_idx);
                }

                player_ptr->oldpy = y;
                player_ptr->oldpx = x;
            }
        }

        player_ptr->teleport_town = false;
    } else if (floor.is_leaving_dungeon()) {
        for (auto y = 0; y < floor.height; y++) {
            for (auto x = 0; x < floor.width; x++) {
                auto &grid = floor.get_grid({ y, x });
                if (!grid.has(TerrainCharacteristics::ENTRANCE)) {
                    continue;
                }

                if (grid.has_monster()) {
                    delete_monster_idx(player_ptr, grid.m_idx);
                }

                player_ptr->oldpy = y;
                player_ptr->oldpx = x;
            }
        }

        player_ptr->teleport_town = false;
    }

    player_place(player_ptr, player_ptr->oldpy, player_ptr->oldpx);
    generate_wild_monsters(player_ptr);
    if (generate_encounter) {
        player_ptr->ambush_flag = true;
    }

    generate_encounter = false;
    auto &quests = QuestList::get_instance();
    for (auto &[quest_id, quest] : quests) {
        if (quest.status == QuestStatusType::REWARDED) {
            quest.status = QuestStatusType::FINISHED;
        }
    }
}

/*!
 * @brief 広域マップの生成(簡易処理版) /
 * Build the wilderness area. -DG-
 */
void wilderness_gen_small(PlayerType *player_ptr)
{
    auto &floor = *player_ptr->current_floor_ptr;
    for (auto x = 0; x < MAX_WID; x++) {
        for (auto y = 0; y < MAX_HGT; y++) {
            floor.get_grid({ y, x }).set_terrain_id(TerrainTag::PERMANENT_WALL);
        }
    }

    const auto &dungeons = DungeonList::get_instance();
    const auto &world = AngbandWorld::get_instance();
    parse_fixed_map(player_ptr, WILDERNESS_DEFINITION, 0, 0, world.max_wild_y, world.max_wild_x);
    for (auto x = 0; x < world.max_wild_x; x++) {
        for (auto y = 0; y < world.max_wild_y; y++) {
            const Pos2D pos(y, x);
            auto &grid = floor.get_grid(pos);
            auto &wild_grid = wilderness[y][x];
            if (wild_grid.town && (wild_grid.town != VALID_TOWNS)) {
                grid.set_terrain_id(TerrainTag::TOWN);
                grid.special = wild_grid.town;
                grid.info |= (CAVE_GLOW | CAVE_MARK);
                continue;
            }

            if (wild_grid.road) {
                grid.set_terrain_id(TerrainTag::FLOOR);
                grid.info |= (CAVE_GLOW | CAVE_MARK);
                continue;
            }

            const auto entrance = wild_grid.entrance;
            if ((entrance > DungeonId::WILDERNESS) && (world.total_winner || dungeons.get_dungeon(entrance).flags.has_not(DungeonFeatureType::WINNER))) {
                grid.set_terrain_id(TerrainTag::ENTRANCE);
                grid.special = static_cast<short>(entrance);
                grid.info |= (CAVE_GLOW | CAVE_MARK);
                continue;
            }

            grid.set_terrain_id(WT_TT_MAP.at(wild_grid.terrain));
            grid.info |= (CAVE_GLOW | CAVE_MARK);
        }
    }

    floor.height = (int16_t)world.max_wild_y;
    floor.width = (int16_t)world.max_wild_x;
    if (floor.height > MAX_HGT) {
        floor.height = MAX_HGT;
    }

    if (floor.width > MAX_WID) {
        floor.width = MAX_WID;
    }

    panel_row_min = floor.height;
    panel_col_min = floor.width;
    player_ptr->x = player_ptr->wilderness_x;
    player_ptr->y = player_ptr->wilderness_y;
    player_ptr->town_num = 0;
}

/*!
 * @brief WildernessDefinition.txt を1行読み取って解析する
 * @param line 読み取ったデータ行のバッファ
 * @param xmin 広域地形マップを読み込みたいx座標の開始位置
 * @param xmax 広域地形マップを読み込みたいx座標の終了位置
 * @param pos_parsing 解析対象の座標
 * @return エラーコードと座標のペア。エラー時は座標は無効値。Dタグの時だけ更新の可能性があり、それ以外はpos_parsingをそのまま返却
 */
std::pair<parse_error_type, std::optional<Pos2D>> parse_line_wilderness(PlayerType *player_ptr, char *line, int xmin, int xmax, const Pos2D &pos_parsing)
{
    if (wilderness_letters.empty()) {
        wilderness_letters.resize(TerrainList::get_instance().size());
    }

    if (!(std::string_view(line).starts_with("W:"))) {
        return { PARSE_ERROR_GENERIC, std::nullopt };
    }

    Pos2D pos = pos_parsing;
    switch (line[2]) {
        /* Process "W:F:<letter>:<terrain>:<town>:<road>:<name> */
#ifdef JP
    case 'E':
        return { PARSE_ERROR_NONE, pos_parsing };
    case 'F':
    case 'J':
#else
    case 'J':
        return { PARSE_ERROR_NONE, pos_parsing };
    case 'F':
    case 'E':
#endif
    {
        char *zz[33];
        const auto num = tokenize(line + 4, 6, zz, 0);
        if (num <= 1) {
            return { PARSE_ERROR_TOO_FEW_ARGUMENTS, std::nullopt };
        }

        int index = zz[0][0];
        auto &letter = wilderness_letters.at(index);
        if (num > 1) {
            letter.terrain = i2enum<WildernessTerrain>(std::stoi(zz[1]));
        } else {
            letter.terrain = WildernessTerrain::EDGE;
        }

        if (num > 2) {
            letter.level = std::stoi(zz[2]);
        } else {
            letter.level = 0;
        }

        if (num > 3) {
            letter.town = static_cast<short>(std::stoi(zz[3]));
        } else {
            letter.town = 0;
        }

        if (num > 4) {
            letter.road = std::stoi(zz[4]);
        } else {
            letter.road = 0;
        }

        if (num > 5) {
            letter.name = zz[5];
        }

        break;
    }

    /* Process "W:D:<layout> */
    /* Layout of the wilderness */
    case 'D': {
        pos.x = xmin;
        char *s = line + 4;
        int len = strlen(s);
        for (auto i = 0; ((pos.x < xmax) && (i < len)); pos.x++, s++, i++) {
            int id = s[0];
            auto &wg = wilderness[pos.y][pos.x];
            const auto &letter = wilderness_letters.at(id);
            wg.terrain = letter.terrain;
            wg.level = letter.level;
            wg.town = letter.town;
            wg.road = letter.road;
            towns_info[letter.town].name = letter.name;
        }

        pos.y++;
        break;
    }

    /* Process "W:P:<x>:<y> - starting position in the wilderness */
    case 'P': {
        auto has_player_located = player_ptr->wilderness_x > 0;
        has_player_located &= player_ptr->wilderness_y > 0;
        if (has_player_located) {
            break;
        }

        char *zz[33];
        if (tokenize(line + 4, 2, zz, 0) != 2) {
            return { PARSE_ERROR_TOO_FEW_ARGUMENTS, std::nullopt };
        }

        player_ptr->wilderness_y = std::stoi(zz[0]);
        player_ptr->wilderness_x = std::stoi(zz[1]);

        auto out_of_bounds = (player_ptr->wilderness_x < 1);
        const auto &world = AngbandWorld::get_instance();
        out_of_bounds |= (player_ptr->wilderness_x > world.max_wild_x);
        out_of_bounds |= (player_ptr->wilderness_y < 1);
        out_of_bounds |= (player_ptr->wilderness_y > world.max_wild_y);
        if (out_of_bounds) {
            return { PARSE_ERROR_OUT_OF_BOUNDS, std::nullopt };
        }

        break;
    }
    default:
        return { PARSE_ERROR_UNDEFINED_DIRECTIVE, std::nullopt };
    }

    for (const auto &[dungeon_id, dungeon] : DungeonList::get_instance()) {
        if (dungeon_id == DungeonId::WILDERNESS) {
            continue;
        }

        wilderness[dungeon->dy][dungeon->dx].entrance = dungeon_id;
        if (!wilderness[dungeon->dy][dungeon->dx].town) {
            wilderness[dungeon->dy][dungeon->dx].level = dungeon->mindepth;
        }
    }

    return { PARSE_ERROR_NONE, pos };
}

/*!
 * @brief ゲーム開始時に各荒野フロアの乱数シードを指定する /
 * Generate the random seeds for the wilderness
 */
void seed_wilderness()
{
    const auto &world = AngbandWorld::get_instance();
    for (auto x = 0; x < world.max_wild_x; x++) {
        for (auto y = 0; y < world.max_wild_y; y++) {
            wilderness[y][x].seed = randint0(0x10000000);
            wilderness[y][x].entrance = DungeonId::WILDERNESS;
        }
    }
}

/*!
 * @brief 荒野の地勢設定を初期化する
 */
void init_wilderness_terrains()
{
    /// @details 地上フロアの種類をキーに、地形タグと出現率 (1/18ずつ)のペアを値にした(擬似的)連想配列.
    /// map にするとTerrainTag の順番がソートされてしまうので不適.
    static const std::map<WildernessTerrain, std::vector<std::pair<TerrainTag, int>>> wt_tag_map{
        { WildernessTerrain::EDGE, { { TerrainTag::PERMANENT_WALL, 18 } } },
        { WildernessTerrain::TOWN, { { TerrainTag::FLOOR, 18 } } },
        { WildernessTerrain::DEEP_WATER, { { TerrainTag::DEEP_WATER, 12 }, { TerrainTag::SHALLOW_WATER, 6 } } },
        { WildernessTerrain::SHALLOW_WATER, { { TerrainTag::DEEP_WATER, 3 }, { TerrainTag::SHALLOW_WATER, 12 }, { TerrainTag::FLOOR, 1 }, { TerrainTag::DIRT, 1 }, { TerrainTag::GRASS, 1 } } },
        { WildernessTerrain::SWAMP, { { TerrainTag::DIRT, 2 }, { TerrainTag::GRASS, 3 }, { TerrainTag::TREE, 1 }, { TerrainTag::BRAKE, 1 }, { TerrainTag::SHALLOW_WATER, 4 }, { TerrainTag::SWAMP, 7 } } },
        { WildernessTerrain::DIRT, { { TerrainTag::FLOOR, 3 }, { TerrainTag::DIRT, 10 }, { TerrainTag::FLOWER, 1 }, { TerrainTag::BRAKE, 1 }, { TerrainTag::GRASS, 1 }, { TerrainTag::TREE, 2 } } },
        { WildernessTerrain::GRASS, { { TerrainTag::FLOOR, 2 }, { TerrainTag::DIRT, 2 }, { TerrainTag::GRASS, 9 }, { TerrainTag::FLOWER, 1 }, { TerrainTag::BRAKE, 2 }, { TerrainTag::TREE, 2 } } },
        { WildernessTerrain::TREES, { { TerrainTag::FLOOR, 2 }, { TerrainTag::DIRT, 1 }, { TerrainTag::TREE, 11 }, { TerrainTag::BRAKE, 2 }, { TerrainTag::GRASS, 2 } } },
        { WildernessTerrain::DESERT, { { TerrainTag::FLOOR, 2 }, { TerrainTag::DIRT, 13 }, { TerrainTag::GRASS, 3 } } },
        { WildernessTerrain::SHALLOW_LAVA, { { TerrainTag::SHALLOW_LAVA, 14 }, { TerrainTag::DEEP_LAVA, 3 }, { TerrainTag::MOUNTAIN, 1 } } },
        { WildernessTerrain::DEEP_LAVA, { { TerrainTag::DIRT, 3 }, { TerrainTag::SHALLOW_LAVA, 3 }, { TerrainTag::DEEP_LAVA, 10 }, { TerrainTag::MOUNTAIN, 2 } } },
        { WildernessTerrain::MOUNTAIN, { { TerrainTag::FLOOR, 1 }, { TerrainTag::BRAKE, 1 }, { TerrainTag::GRASS, 2 }, { TerrainTag::DIRT, 2 }, { TerrainTag::TREE, 2 }, { TerrainTag::MOUNTAIN, 10 } } },
    };

    for (const auto &[wt, tags] : wt_tag_map) {
        const auto check = std::accumulate(tags.begin(), tags.end(), 0, [](int sum, const auto &x) { return sum + x.second; });
        if (check != SUM_TERRAIN_PROBABILITIES) {
            THROW_EXCEPTION(std::logic_error, "Initializing wilderness is failed!");
        }

        terrain_table.emplace(wt, std::map<short, TerrainTag>());
        short cur = 0;
        for (const auto &[tag, num] : tags) {
            const auto limit = cur + num;
            for (; (cur < limit) && (cur < SUM_TERRAIN_PROBABILITIES); cur++) {
                terrain_table.at(wt).emplace(cur, tag);
            }
        }
    }
}

void init_wilderness_encounter()
{
    generate_encounter = false;
}

/*!
 * @brief 荒野から広域マップへの切り替え処理 /
 * Initialize arrays for wilderness terrains
 * @param encount 襲撃時TRUE
 * @return 切り替えが行われた場合はTRUEを返す。
 */
bool change_wild_mode(PlayerType *player_ptr, bool encount)
{
    generate_encounter = encount;
    if (player_ptr->leaving) {
        return false;
    }

    if (lite_town || vanilla_town) {
        msg_print(_("荒野なんてない。", "No global map."));
        return false;
    }

    auto &world = AngbandWorld::get_instance();
    if (world.is_wild_mode()) {
        player_ptr->wilderness_x = player_ptr->x;
        player_ptr->wilderness_y = player_ptr->y;
        player_ptr->energy_need = 0;
        world.set_wild_mode(false);
        player_ptr->leaving = true;
        return true;
    }

    bool has_pet = false;
    PlayerEnergy energy(player_ptr);
    for (int i = 1; i < player_ptr->current_floor_ptr->m_max; i++) {
        auto *m_ptr = &player_ptr->current_floor_ptr->m_list[i];
        if (!m_ptr->is_valid()) {
            continue;
        }

        if (m_ptr->is_pet() && !m_ptr->is_riding()) {
            has_pet = true;
        }

        if (m_ptr->is_asleep() || (m_ptr->cdis > MAX_PLAYER_SIGHT) || !m_ptr->is_hostile()) {
            continue;
        }

        msg_print(_("敵がすぐ近くにいるときは広域マップに入れない！", "You cannot enter global map, since there are some monsters nearby!"));
        energy.reset_player_turn();
        return false;
    }

    if (has_pet) {
        concptr msg = _("ペットを置いて広域マップに入りますか？", "Do you leave your pets behind? ");
        if (!input_check_strict(player_ptr, msg, UserCheck::OKAY_CANCEL)) {
            energy.reset_player_turn();
            return false;
        }
    }

    energy.set_player_turn_energy(1000);
    player_ptr->oldpx = player_ptr->x;
    player_ptr->oldpy = player_ptr->y;
    SpellHex spell_hex(player_ptr);
    if (spell_hex.is_spelling_any()) {
        spell_hex.stop_all_spells();
    }

    set_action(player_ptr, ACTION_NONE);
    world.set_wild_mode(true);
    player_ptr->leaving = true;
    return true;
}
