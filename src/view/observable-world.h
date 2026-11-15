#pragma once

/*!
 * @file observable-world.h
 * @brief プレイヤーが知り得るフロアの状態 (表示非依存のワールドモデル)
 * @details
 * 端末の text_hook / pict_hook は「画面上のどのセルに何の文字・タイルを描くか」しか伝えない。
 * 3D 表示などの別の表現を作るには「フロアのどの座標にどの地形・クリーチャーがあるか」が要るため、
 * それをゲームの表示規則 (記憶・視界・幻覚) に従って取り出す。
 *
 * - 地形は is_map_terrain_visible() が真のグリッドだけを渡す (未知のグリッドは UNKNOWN)
 * - クリーチャーは is_visible_on_map() が真のものだけを渡す
 * - 幻覚中は種族・体力・状態を伏せる
 * - 各グリッドの表示シンボルは map_info() の結果をそのまま持つ (端末版と同じ見た目の手掛かり)
 *
 * よって端末版の画面より多くの情報は含まない。
 */

#include "util/point-2d.h"
#include <cstdint>
#include <vector>

class CreatureEntity;

/*!
 * @brief グリッドの形状の大分類 (3D 表示で何を置くかの目安)
 * @details 値は外部 (Godot) へそのまま渡すため、追加は末尾に限る。
 */
enum class ObservedTerrainShape : uint8_t {
    UNKNOWN = 0, //!< 未知のグリッド
    FLOOR = 1, //!< 床
    WALL = 2, //!< 壁
    DOOR_CLOSED = 3, //!< 閉じた扉
    DOOR_OPEN = 4, //!< 開いた扉・壊れた扉
    STAIRS_UP = 5, //!< 上り階段
    STAIRS_DOWN = 6, //!< 下り階段
    LIQUID = 7, //!< 水・溶岩・毒溜まり
    TREE = 8, //!< 木・植物
    ENTRANCE = 9, //!< 店・施設の入口
    OTHER_PASSABLE = 10, //!< 上記以外の通れる地形
    OTHER_OBSTACLE = 11, //!< 上記以外の通れない地形
};

/*!
 * @brief グリッドの観測フラグ (ビット)
 * @details 値は外部 (Godot) へそのまま渡すため、追加は空きビットに限る。
 */
enum ObservedGridFlag : uint8_t {
    OBSERVED_GRID_KNOWN = 1U << 0, //!< 地形が分かっている (記憶済み、または今見えている)
    OBSERVED_GRID_IN_VIEW = 1U << 1, //!< 視界内 (CAVE_VIEW)
    OBSERVED_GRID_LIT = 1U << 2, //!< 明るい (光源・部屋の照明)
    OBSERVED_GRID_OBJECT = 1U << 3, //!< 見つけた床アイテムがある
    OBSERVED_GRID_CORPSE = 1U << 4, //!< 死体がある
    OBSERVED_GRID_MONSTER = 1U << 5, //!< 見えているクリーチャーがいる
    OBSERVED_GRID_PLAYER = 1U << 6, //!< プレイヤーがいる
};

/*!
 * @brief 1 グリッドの観測結果
 */
struct ObservedGrid {
    short terrain_id = -1; //!< 見かけの地形 ID (ミミック考慮後)。未知なら -1
    ObservedTerrainShape shape = ObservedTerrainShape::UNKNOWN;
    uint8_t flags = 0; //!< ObservedGridFlag の論理和
    uint8_t fg_color = 0; //!< map_info() の前景色 (タイル表示時はタイルの行)
    uint8_t fg_char = 0; //!< map_info() の前景文字 (タイル表示時はタイルの列)
    uint8_t bg_color = 0; //!< map_info() の背景色
    uint8_t bg_char = 0; //!< map_info() の背景文字

    bool operator==(const ObservedGrid &other) const = default;
};

/*!
 * @brief クリーチャーの体力の段階 (端末版の体力表示と同じ区切り)
 */
enum class ObservedHealth : uint8_t {
    UNKNOWN = 0, //!< 分からない (幻覚中)
    UNHURT = 1,
    LIGHTLY_WOUNDED = 2, //!< 60% 以上
    WOUNDED = 3, //!< 25% 以上
    BADLY_WOUNDED = 4, //!< 10% 以上
    ALMOST_DEAD = 5,
};

/*!
 * @brief クリーチャーの観測フラグ (ビット)
 */
enum ObservedCreatureFlag : uint16_t {
    OBSERVED_CREATURE_IN_VIEW = 1U << 0, //!< 視界内で直接見えている (感知のみなら偽)
    OBSERVED_CREATURE_HALLUCINATED = 1U << 1, //!< 幻覚で正体が分からない
    OBSERVED_CREATURE_ASLEEP = 1U << 2,
    OBSERVED_CREATURE_STUNNED = 1U << 3,
    OBSERVED_CREATURE_CONFUSED = 1U << 4,
    OBSERVED_CREATURE_FEARFUL = 1U << 5,
    OBSERVED_CREATURE_PET = 1U << 6,
    OBSERVED_CREATURE_FRIENDLY = 1U << 7,
    OBSERVED_CREATURE_RIDDEN = 1U << 8, //!< プレイヤーが騎乗している
};

/*!
 * @brief 1 体のクリーチャーの観測結果
 */
struct ObservedCreature {
    uint32_t display_id = 0; //!< 表示用の識別番号 (CreatureEntity::get_display_id())。同じ個体の移動を追うのに使う
    short m_idx = 0; //!< フロアのモンスター配列の添字 (フロア内・圧縮までの間だけ一意)
    Pos2D pos{ 0, 0 };
    short race_id = -1; //!< 見かけの種族 ID。幻覚中は -1
    ObservedHealth health = ObservedHealth::UNKNOWN;
    uint16_t flags = 0; //!< ObservedCreatureFlag の論理和
};

/*!
 * @brief フロア全体の観測結果
 */
struct ObservableWorld {
    bool is_valid = false; //!< フロアが存在しない (ゲーム開始前等) なら偽
    int width = 0;
    int height = 0;
    short dungeon_id = 0;
    short dun_level = 0;
    bool is_wild_mode = false; //!< 広域マップ表示中
    Pos2D player_pos{ 0, 0 };
    std::vector<ObservedGrid> grids; //!< width × height (行優先)
    std::vector<ObservedCreature> creatures;

    const ObservedGrid &get_grid(const Pos2D &pos) const;
};

ObservedTerrainShape classify_observed_terrain(short terrain_id);
ObservedHealth classify_observed_health(int hp, int maxhp);

/// フロア全体の観測結果を作る
ObservableWorld build_observable_world(CreatureEntity &player, bool with_symbols = true);

/// フロア全体以外 (地形・クリーチャー以外) の観測結果だけを作る。grids / creatures は空
ObservableWorld build_observable_world_header(CreatureEntity &player);

/// 1 グリッドの観測結果を作る (変わったグリッドだけを求め直すとき用)。
/// with_symbols が真なら、事前に MapInfoCache::prepare() を呼んでおくこと
ObservedGrid observe_grid(CreatureEntity &player, const Pos2D &pos, bool with_symbols = true);

/// 見えているクリーチャーの観測結果を作る
std::vector<ObservedCreature> observe_creatures(CreatureEntity &player);
