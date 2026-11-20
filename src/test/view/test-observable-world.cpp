#include "game-option/map-screen-options.h"
#include "system/creature-timed-effect-types.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "test/scoped-restore.h"
#include "util/finalizer.h"
#include "view/observable-world.h"
#include "view/view-observer.h"
#include "world/world.h"
#include <doctest/doctest.h>
#include <memory>
#include <vector>

namespace {
struct TestTerrains {
    short floor_id;
    short wall_id;
    short door_id;
    short open_door_id;
    short down_stairs_id;
};

TestTerrains add_test_terrains(TerrainList &terrains, size_t old_size)
{
    terrains.resize(old_size + 5);
    const TestTerrains ids{
        static_cast<short>(old_size),
        static_cast<short>(old_size + 1),
        static_cast<short>(old_size + 2),
        static_cast<short>(old_size + 3),
        static_cast<short>(old_size + 4),
    };
    const auto define = [&terrains](short id, EnumClassFlagGroup<TerrainCharacteristics> flags) {
        auto &terrain = terrains.get_terrain(id);
        terrain.idx = id;
        terrain.mimic = id;
        terrain.flags = flags;
    };
    using TC = TerrainCharacteristics;
    define(ids.floor_id, { TC::FLOOR, TC::LOS, TC::PROJECTION, TC::MOVE });
    define(ids.wall_id, { TC::WALL, TC::REMEMBER });
    define(ids.door_id, { TC::DOOR, TC::OPEN, TC::REMEMBER });
    define(ids.open_door_id, { TC::DOOR, TC::CLOSE, TC::MOVE, TC::LOS, TC::PROJECTION, TC::REMEMBER });
    define(ids.down_stairs_id, { TC::DOWN_STAIRS, TC::STAIRS, TC::MOVE, TC::LOS, TC::PROJECTION, TC::REMEMBER });
    return ids;
}
}

TEST_CASE("Observed terrain shapes follow the terrain characteristics")
{
    auto &terrains = TerrainList::get_instance();
    const auto old_size = terrains.size();
    const auto restore = util::make_finalizer([&terrains, old_size] { terrains.resize(old_size); });
    const auto ids = add_test_terrains(terrains, old_size);

    CHECK(classify_observed_terrain(-1) == ObservedTerrainShape::UNKNOWN);
    CHECK(classify_observed_terrain(ids.floor_id) == ObservedTerrainShape::FLOOR);
    CHECK(classify_observed_terrain(ids.wall_id) == ObservedTerrainShape::WALL);
    CHECK(classify_observed_terrain(ids.door_id) == ObservedTerrainShape::DOOR_CLOSED);
    CHECK(classify_observed_terrain(ids.open_door_id) == ObservedTerrainShape::DOOR_OPEN);
    CHECK(classify_observed_terrain(ids.down_stairs_id) == ObservedTerrainShape::STAIRS_DOWN);

    // 見つけた罠は床の罠でも落とし戸でも罠として分ける (床の板だけでは見分けられないため)
    const auto trap_id = static_cast<short>(terrains.size());
    terrains.resize(terrains.size() + 1);
    auto &trap = terrains.get_terrain(trap_id);
    trap.idx = trap_id;
    trap.mimic = trap_id;
    trap.flags = { TerrainCharacteristics::TRAP, TerrainCharacteristics::FLOOR, TerrainCharacteristics::MOVE };
    CHECK(classify_observed_terrain(trap_id) == ObservedTerrainShape::TRAP);
    trap.flags = { TerrainCharacteristics::TRAP, TerrainCharacteristics::MOVE };
    CHECK(classify_observed_terrain(trap_id) == ObservedTerrainShape::TRAP);
}

TEST_CASE("Observed health bands match the monster health display")
{
    CHECK(classify_observed_health(100, 100) == ObservedHealth::UNHURT);
    CHECK(classify_observed_health(60, 100) == ObservedHealth::LIGHTLY_WOUNDED);
    CHECK(classify_observed_health(59, 100) == ObservedHealth::WOUNDED);
    CHECK(classify_observed_health(25, 100) == ObservedHealth::WOUNDED);
    CHECK(classify_observed_health(24, 100) == ObservedHealth::BADLY_WOUNDED);
    CHECK(classify_observed_health(10, 100) == ObservedHealth::BADLY_WOUNDED);
    CHECK(classify_observed_health(9, 100) == ObservedHealth::ALMOST_DEAD);
    CHECK(classify_observed_health(0, 0) == ObservedHealth::UNHURT);
}

TEST_CASE("The observable world discloses only what the player knows")
{
    auto &terrains = TerrainList::get_instance();
    auto &world = AngbandWorld::get_instance();
    const auto old_size = terrains.size();
    const auto restore_options = test::scoped_restore(view_hidden_walls);
    const auto restore = util::make_finalizer([&terrains, &world, old_size, wild = world.is_wild_mode()] {
        terrains.resize(old_size);
        world.set_wild_mode(wild);
    });
    const auto ids = add_test_terrains(terrains, old_size);
    world.set_wild_mode(false);
    view_hidden_walls = true;

    auto floor_ptr = std::make_unique<FloorType>();
    auto &floor = *floor_ptr;
    floor.width = 4;
    floor.height = 3;
    floor.dun_level = 7;
    for (const auto &pos : floor.get_area()) {
        auto &grid = floor.get_grid(pos);
        grid.feat = ids.wall_id;
        grid.mimic = 0;
        grid.info = 0;
    }

    auto player_ptr = std::make_unique<PlayerType>();
    auto &player = *player_ptr;
    player.set_floor(floor_ptr.get());
    player.y = 1;
    player.x = 1;

    // 記憶した壁、視界内で照らされた床、何も知らない壁を 1 つずつ用意する
    const Pos2D remembered_wall{ 0, 0 };
    const Pos2D lit_floor{ 1, 1 };
    const Pos2D unknown_wall{ 2, 3 };
    floor.get_grid(remembered_wall).info = CAVE_MARK | CAVE_KNOWN;
    auto &floor_grid = floor.get_grid(lit_floor);
    floor_grid.feat = ids.floor_id;
    floor_grid.info = CAVE_VIEW | CAVE_LITE | CAVE_KNOWN;

    const auto observed = build_observable_world(player, false);
    REQUIRE(observed.is_valid);
    CHECK(observed.width == 4);
    CHECK(observed.height == 3);
    CHECK(observed.dun_level == 7);
    CHECK(observed.grids.size() == 12);
    CHECK(observed.player_pos == lit_floor);
    CHECK(observed.creatures.empty());
    CHECK_FALSE(observed.player_blind);
    CHECK_FALSE(observed.player_hallucinated);

    const auto &wall = observed.get_grid(remembered_wall);
    CHECK(wall.terrain_id == ids.wall_id);
    CHECK(wall.shape == ObservedTerrainShape::WALL);
    CHECK((wall.flags & OBSERVED_GRID_KNOWN) != 0);
    CHECK((wall.flags & OBSERVED_GRID_IN_VIEW) == 0);

    const auto &lit = observed.get_grid(lit_floor);
    CHECK(lit.terrain_id == ids.floor_id);
    CHECK(lit.shape == ObservedTerrainShape::FLOOR);
    CHECK((lit.flags & OBSERVED_GRID_KNOWN) != 0);
    CHECK((lit.flags & OBSERVED_GRID_IN_VIEW) != 0);
    CHECK((lit.flags & OBSERVED_GRID_LIT) != 0);
    CHECK((lit.flags & OBSERVED_GRID_PLAYER) != 0);

    // 知らない地形は ID も形状も伏せる
    const auto &unknown = observed.get_grid(unknown_wall);
    CHECK(unknown.terrain_id == -1);
    CHECK(unknown.shape == ObservedTerrainShape::UNKNOWN);
    CHECK(unknown.flags == 0);

    SUBCASE("Re-observing one grid gives the same result as the full build")
    {
        for (const auto &pos : { remembered_wall, lit_floor, unknown_wall }) {
            CHECK(observe_grid(player, pos, false) == observed.get_grid(pos));
        }

        // 忘れたグリッドは個別に求め直しても伏せられる
        floor.get_grid(remembered_wall).info = 0;
        CHECK(observe_grid(player, remembered_wall, false).terrain_id == -1);
    }
    SUBCASE("Blindness hides terrain that is not remembered")
    {
        player.set_timed_effect(CreatureTimedEffect::BLINDNESS, 10);
        const auto blind = build_observable_world(player, false);
        CHECK(blind.player_blind);
        CHECK(blind.get_grid(lit_floor).terrain_id == -1);
        CHECK(blind.get_grid(remembered_wall).terrain_id == ids.wall_id);
        player.set_timed_effect(CreatureTimedEffect::BLINDNESS, 0);
    }
}

TEST_CASE("View observer receives grid and whole-map notifications")
{
    struct RecordingObserver : public ViewObserver {
        std::vector<Pos2D> grids;
        int all_count = 0;
        void on_grid_changed(const Pos2D &pos) override
        {
            this->grids.push_back(pos);
        }
        void on_all_grids_changed() override
        {
            this->all_count++;
        }
    };

    RecordingObserver observer;
    const auto restore = util::make_finalizer([] { set_view_observer(nullptr); });

    // 未登録なら何も起きない
    notify_view_grid_changed({ 1, 2 });
    notify_view_all_grids_changed();

    set_view_observer(&observer);
    notify_view_grid_changed({ 3, 4 });
    notify_view_all_grids_changed();
    REQUIRE(observer.grids.size() == 1);
    CHECK(observer.grids[0] == Pos2D(3, 4));
    CHECK(observer.all_count == 1);

    set_view_observer(nullptr);
    notify_view_grid_changed({ 5, 6 });
    CHECK(observer.grids.size() == 1);
}

TEST_CASE("Display IDs are stable across moves and reset by wipe")
{
    CreatureEntity first;
    CreatureEntity second;
    const auto first_id = first.get_display_id();
    const auto second_id = second.get_display_id();
    CHECK(first_id != 0);
    CHECK(second_id != 0);
    CHECK(first_id != second_id);
    CHECK(first.get_display_id() == first_id);

    // モンスター配列の圧縮と同じく実体ごと移すと番号も移る
    CreatureEntity moved = std::move(first);
    CHECK(moved.get_display_id() == first_id);

    // 空いた添字を再利用するときの wipe() で番号は付け直される
    moved.wipe();
    const auto reused_id = moved.get_display_id();
    CHECK(reused_id != 0);
    CHECK(reused_id != first_id);
    CHECK(reused_id != second_id);
}

TEST_CASE("The terrain catalog lists tags, shapes and default symbols")
{
    auto &terrains = TerrainList::get_instance();
    const auto old_size = terrains.size();
    const auto restore = util::make_finalizer([&terrains, old_size] { terrains.resize(old_size); });
    const auto ids = add_test_terrains(terrains, old_size);
    auto &wall = terrains.get_terrain(ids.wall_id);
    wall.tag = "TEST_WALL";
    wall.symbol_definitions[F_LIT_STANDARD] = { 7, '#' };

    const auto catalog = build_terrain_catalog();
    REQUIRE(catalog.size() == terrains.size());
    const auto &entry = catalog[static_cast<size_t>(ids.wall_id)];
    CHECK(entry.id == ids.wall_id);
    CHECK(entry.tag == "TEST_WALL");
    CHECK(entry.shape == ObservedTerrainShape::WALL);
    CHECK(entry.symbol_char == '#');
    CHECK(entry.symbol_color == 7);
}

TEST_CASE("The target path stops only at walls the player remembers")
{
    auto &terrains = TerrainList::get_instance();
    auto &world = AngbandWorld::get_instance();
    const auto old_size = terrains.size();
    const auto restore_options = test::scoped_restore(view_hidden_walls);
    const auto restore = util::make_finalizer([&terrains, &world, old_size, wild = world.is_wild_mode()] {
        terrains.resize(old_size);
        world.set_wild_mode(wild);
    });
    const auto ids = add_test_terrains(terrains, old_size);
    world.set_wild_mode(false);
    view_hidden_walls = true;

    // 8 x 3 のフロアの真ん中の行を床にし、x = 4 に壁を置く
    auto floor_ptr = std::make_unique<FloorType>();
    auto &floor = *floor_ptr;
    floor.width = 8;
    floor.height = 3;
    for (const auto &pos : floor.get_area()) {
        auto &grid = floor.get_grid(pos);
        grid.feat = (pos.y == 1) ? ids.floor_id : ids.wall_id;
        grid.mimic = 0;
        grid.info = 0;
    }
    const Pos2D wall_pos{ 1, 4 };
    floor.get_grid(wall_pos).feat = ids.wall_id;

    auto player_ptr = std::make_unique<PlayerType>();
    auto &player = *player_ptr;
    player.set_floor(floor_ptr.get());
    player.y = 1;
    player.x = 1;
    const Pos2D cursor{ 1, 6 };

    // プレイヤー自身のマスを選んだら射線は無い
    CHECK(observe_target_path(player, player.get_position()).grids.empty());

    // 知らない壁では止めない (壁の位置を明かさない)
    const auto unknown = observe_target_path(player, cursor);
    CHECK_FALSE(unknown.blocked);
    REQUIRE_FALSE(unknown.grids.empty());
    CHECK(unknown.grids.front() == Pos2D(1, 2));
    CHECK(unknown.grids.back() == cursor);
    CHECK(unknown.grids.size() == 5);
    CHECK_FALSE(unknown.creature_index);

    // 記憶している壁では、その壁のマスで止まる
    floor.get_grid(wall_pos).info = CAVE_MARK | CAVE_KNOWN;
    const auto known = observe_target_path(player, cursor);
    CHECK(known.blocked);
    REQUIRE(known.grids.size() == 3);
    CHECK(known.grids.back() == wall_pos);
}
