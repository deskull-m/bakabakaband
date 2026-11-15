#include "view/observable-world.h"
#include "system/creature-entity.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include "view/display-map.h"
#include "view/map-info-cache.h"
#include "world/world.h"

const ObservedGrid &ObservableWorld::get_grid(const Pos2D &pos) const
{
    return this->grids.at(static_cast<size_t>(pos.y * this->width + pos.x));
}

/*!
 * @brief 地形 ID から形状の大分類を求める
 * @param terrain_id 見かけの地形 ID (ミミック考慮後)。負なら未知
 */
ObservedTerrainShape classify_observed_terrain(short terrain_id)
{
    if (terrain_id < 0) {
        return ObservedTerrainShape::UNKNOWN;
    }

    const auto &flags = TerrainList::get_instance().get_terrain(terrain_id).flags;
    if (flags.has(TerrainCharacteristics::UP_STAIRS)) {
        return ObservedTerrainShape::STAIRS_UP;
    }
    if (flags.has(TerrainCharacteristics::DOWN_STAIRS)) {
        return ObservedTerrainShape::STAIRS_DOWN;
    }
    if (flags.has(TerrainCharacteristics::DOOR)) {
        // 閉じた扉は開けられ、開いた扉は閉じられる。壊れた扉はどちらでもなく通れる
        return flags.has(TerrainCharacteristics::MOVE) ? ObservedTerrainShape::DOOR_OPEN : ObservedTerrainShape::DOOR_CLOSED;
    }
    if (flags.has_any_of({ TerrainCharacteristics::STORE, TerrainCharacteristics::BLDG })) {
        return ObservedTerrainShape::ENTRANCE;
    }
    if (flags.has(TerrainCharacteristics::WALL)) {
        return ObservedTerrainShape::WALL;
    }
    if (flags.has_any_of({ TerrainCharacteristics::WATER, TerrainCharacteristics::LAVA, TerrainCharacteristics::POISON_PUDDLE })) {
        return ObservedTerrainShape::LIQUID;
    }
    if (flags.has_any_of({ TerrainCharacteristics::TREE, TerrainCharacteristics::PLANT })) {
        return ObservedTerrainShape::TREE;
    }
    if (flags.has(TerrainCharacteristics::FLOOR)) {
        return ObservedTerrainShape::FLOOR;
    }

    return flags.has(TerrainCharacteristics::MOVE) ? ObservedTerrainShape::OTHER_PASSABLE : ObservedTerrainShape::OTHER_OBSTACLE;
}

/*!
 * @brief 体力の段階を求める (端末版・ボット出力の体力表示と同じ区切り)
 */
ObservedHealth classify_observed_health(int hp, int maxhp)
{
    if (hp >= maxhp) {
        return ObservedHealth::UNHURT;
    }

    const auto percent = maxhp > 0 ? 100L * hp / maxhp : 0;
    if (percent >= 60) {
        return ObservedHealth::LIGHTLY_WOUNDED;
    }
    if (percent >= 25) {
        return ObservedHealth::WOUNDED;
    }
    if (percent >= 10) {
        return ObservedHealth::BADLY_WOUNDED;
    }

    return ObservedHealth::ALMOST_DEAD;
}

namespace {
bool is_lit_grid(const Grid &grid)
{
    return any_bits(grid.info, CAVE_LITE | CAVE_MNLT) || match_bits(grid.info, CAVE_GLOW | CAVE_MNDK, CAVE_GLOW);
}

bool has_found_object(const FloorType &floor, const Grid &grid)
{
    for (const auto o_idx : grid.o_idx_list) {
        const auto &item = *floor.o_list[o_idx];
        if (item.is_valid() && item.marked.has(OmType::FOUND)) {
            return true;
        }
    }

    return false;
}

}

/*!
 * @brief 1 グリッドの観測結果を作る
 * @param player プレイヤー (視点となるクリーチャー)
 * @param pos 階の中の座標
 * @param with_symbols 表示シンボル (map_info() の結果) も求めるか
 */
ObservedGrid observe_grid(CreatureEntity &player, const Pos2D &pos, bool with_symbols)
{
    const auto &floor = *player.get_floor();
    const auto &grid = floor.get_grid(pos);
    ObservedGrid observed;
    const auto is_known = is_map_terrain_visible(player, pos);
    if (is_known) {
        observed.terrain_id = grid.get_terrain_id(TerrainKind::MIMIC);
        observed.shape = classify_observed_terrain(observed.terrain_id);
        observed.flags |= OBSERVED_GRID_KNOWN;
        if (is_lit_grid(grid)) {
            observed.flags |= OBSERVED_GRID_LIT;
        }

        // 端末版の地図は地形の分からないグリッドの死体も描くが、
        // 3D 表示では地形と切り離して置くことになるので、地形が分かるグリッドに限る
        if (grid.has_corpse()) {
            observed.flags |= OBSERVED_GRID_CORPSE;
        }
    }

    if (grid.is_view()) {
        observed.flags |= OBSERVED_GRID_IN_VIEW;
    }

    // 見つけた床アイテムは地形が分からなくても端末版の地図に描かれる (感知の結果など)
    if (has_found_object(floor, grid)) {
        observed.flags |= OBSERVED_GRID_OBJECT;
    }

    if (player.get_position() == pos) {
        observed.flags |= OBSERVED_GRID_PLAYER;
    }

    if (grid.has_monster() && floor.get_monster(grid.m_idx).is_visible_on_map()) {
        observed.flags |= OBSERVED_GRID_MONSTER;
    }

    if (!with_symbols) {
        return observed;
    }

    const auto symbol_pair = MapInfoCache::get_instance().get(player, pos);
    observed.fg_color = symbol_pair.symbol_foreground.color;
    observed.fg_char = static_cast<uint8_t>(symbol_pair.symbol_foreground.character);
    observed.bg_color = symbol_pair.symbol_background.color;
    observed.bg_char = static_cast<uint8_t>(symbol_pair.symbol_background.character);
    return observed;
}

namespace {

ObservedCreature observe_creature(const CreatureEntity &player, short m_idx, const CreatureEntity &monster)
{
    const auto &floor = *player.get_floor();
    ObservedCreature observed;
    observed.display_id = monster.get_display_id();
    observed.m_idx = m_idx;
    observed.pos = monster.get_position();
    if (floor.get_grid(observed.pos).is_view()) {
        observed.flags |= OBSERVED_CREATURE_IN_VIEW;
    }
    if (player.get_riding() == m_idx) {
        observed.flags |= OBSERVED_CREATURE_RIDDEN;
    }

    // 幻覚中は何かがいることと、敵か味方かしか分からない (端末版の地図・ボット出力と同じ)
    if (player.is_hallucinated()) {
        observed.flags |= OBSERVED_CREATURE_HALLUCINATED;
        if (monster.is_pet()) {
            observed.flags |= OBSERVED_CREATURE_PET;
        }
        return observed;
    }

    observed.race_id = static_cast<short>(enum2i(monster.get_apparent_monrace().idx));
    observed.health = classify_observed_health(monster.hp, monster.maxhp);
    const std::pair<bool, ObservedCreatureFlag> states[] = {
        { monster.is_asleep(), OBSERVED_CREATURE_ASLEEP },
        { monster.is_stunned(), OBSERVED_CREATURE_STUNNED },
        { monster.is_confused(), OBSERVED_CREATURE_CONFUSED },
        { monster.is_fearful(), OBSERVED_CREATURE_FEARFUL },
        { monster.is_pet(), OBSERVED_CREATURE_PET },
        { monster.is_friendly(), OBSERVED_CREATURE_FRIENDLY },
    };
    for (const auto &[is_set, flag] : states) {
        if (is_set) {
            observed.flags |= flag;
        }
    }

    return observed;
}
}

/*!
 * @brief 見えているクリーチャーの観測結果を作る
 */
std::vector<ObservedCreature> observe_creatures(CreatureEntity &player)
{
    std::vector<ObservedCreature> creatures;
    const auto &floor = *player.get_floor();
    for (short m_idx = 1; m_idx < floor.m_max; ++m_idx) {
        const auto &monster = floor.m_list[m_idx];
        if (!monster.is_valid() || !monster.is_visible_on_map()) {
            continue;
        }

        creatures.push_back(observe_creature(player, m_idx, monster));
    }

    return creatures;
}

/*!
 * @brief フロアの大きさ・階層・プレイヤーの位置だけを取り出す
 * @return フロアが無ければ is_valid が偽
 */
ObservableWorld build_observable_world_header(CreatureEntity &player)
{
    ObservableWorld world;
    const auto *floor_ptr = player.get_floor();
    if (floor_ptr == nullptr || floor_ptr->width <= 0 || floor_ptr->height <= 0) {
        return world;
    }

    const auto &floor = *floor_ptr;
    world.is_valid = true;
    world.width = floor.width;
    world.height = floor.height;
    world.dungeon_id = static_cast<short>(enum2i(floor.dungeon_id));
    world.dun_level = static_cast<short>(floor.dun_level);
    world.is_wild_mode = AngbandWorld::get_instance().is_wild_mode();
    world.player_pos = player.get_position();
    return world;
}

/*!
 * @brief プレイヤーが知り得るフロアの状態を取り出す
 * @param player プレイヤー (視点となるクリーチャー)
 * @param with_symbols 各グリッドの表示シンボル (map_info() の結果) も求めるか。
 * 偽なら fg_* / bg_* は 0 のまま (地形の表示定義を持たない単体テスト等のため)
 * @details ゲームの状態は変えない (map_info() は表示用の専用乱数だけを使う)。
 * ゲームスレッドから呼ぶこと。
 */
ObservableWorld build_observable_world(CreatureEntity &player, bool with_symbols)
{
    auto world = build_observable_world_header(player);
    if (!world.is_valid) {
        return world;
    }

    if (with_symbols) {
        MapInfoCache::get_instance().prepare(player);
    }

    world.grids.reserve(static_cast<size_t>(world.width * world.height));
    for (auto y = 0; y < world.height; ++y) {
        for (auto x = 0; x < world.width; ++x) {
            world.grids.push_back(observe_grid(player, { y, x }, with_symbols));
        }
    }

    world.creatures = observe_creatures(player);
    return world;
}
