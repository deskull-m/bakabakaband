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
#include <string>
#include <tl/optional.hpp>
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
    TRAP = 12, //!< 見つけた罠 (床の罠も落とし戸も、文字で見分けられるよう床とは分ける)
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
    uint8_t object_tval = 0; //!< いちばん上の見つけた床アイテムの種別 (ItemKindType)。無い・幻覚中は 0
    uint8_t object_color = 0; //!< いちばん上の見つけた床アイテムの表示色 (タイル表示時はタイルの行)。無ければ 0
    uint8_t object_char = 0; //!< いちばん上の見つけた床アイテムの表示文字 (タイル表示時はタイルの列)。無ければ 0

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
    char symbol_char = 0; //!< 見かけの種族の定義上の文字 (タイル表示の設定に依らない ASCII)。幻覚中は 0
    uint8_t symbol_color = 0; //!< 見かけの種族の定義上の色。幻覚中は 0
    uint8_t light_radius = 0; //!< 周りを照らす光の半径 (HAS_LITE_1/2)。幻覚中・光を持たないなら 0
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
    int player_light_radius = 0; //!< プレイヤーの光源の半径 (get_cur_lite())
    bool player_blind = false; //!< 盲目 (視界のフラグに依らず、見えているのは記憶だけ)
    bool player_hallucinated = false; //!< 幻覚
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

/*!
 * @brief プレイヤーからターゲットのマスまでの射線 (ボルト・矢の軌跡の目安)
 * @details 経路はゲームの投射と同じ直線 (ProjectionPath) で、止まる位置はプレイヤーが知っている
 * ことだけで決める (記憶している地形が投射を通さないマスで止める。知らない壁では止めない)。
 * 見えているクリーチャーがいるマスは、ボルトがそこで当たる目安として最初の 1 つを返す。
 */
struct ObservedTargetPath {
    std::vector<Pos2D> grids; //!< プレイヤーの次のマスから順に (プレイヤー自身のマスは含まない)
    bool blocked = false; //!< 最後のマスが、記憶している投射を通さない地形 (壁など) か
    tl::optional<int> creature_index; //!< 経路上で最初に見えているクリーチャーがいるマスの添字

    bool operator==(const ObservedTargetPath &) const = default;
};

/// プレイヤーから dst までの射線を求める (dst がプレイヤーの位置なら空)
ObservedTargetPath observe_target_path(const CreatureEntity &player, const Pos2D &dst);

/*!
 * @brief 地形の一覧の 1 件 (見た目の定義を地形のタグで書けるようにするため)
 */
struct TerrainCatalogEntry {
    short id = 0;
    std::string tag; //!< 地形のタグ (TerrainDefinitions.jsonc の tag)
    std::string name; //!< 地形の名前 (内部の文字コード)
    ObservedTerrainShape shape = ObservedTerrainShape::UNKNOWN;
    char symbol_char = 0; //!< 定義上の文字 (標準の明るさ)
    uint8_t symbol_color = 0; //!< 定義上の色 (標準の明るさ)
};

/// 全地形の一覧を作る (地形の定義を読み込んだ後に呼ぶこと)
std::vector<TerrainCatalogEntry> build_terrain_catalog();
