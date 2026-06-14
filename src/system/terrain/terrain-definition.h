#pragma once

#include "system/angband.h"
#include "system/enums/store-sale-type.h"
#include "system/enums/terrain/building-type.h"
#include "system/enums/terrain/pattern-tile-type.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/enums/terrain/terrain-conversion-type.h"
#include "system/enums/terrain/trap.h"
#include "util/dice.h"
#include "util/flag-group.h"
#include "view/display-symbol.h"
#include <cstdint>
#include <map>
#include <string>
#include <vector>

/* Number of feats we change to (Excluding default). Used in TerrainDefinitions.txt. */
constexpr auto MAX_FEAT_STATES = 8;

/* Lighting levels of features' attr and char */
constexpr auto F_LIT_STANDARD = 0; /* Standard */
constexpr auto F_LIT_LITE = 1; /* Brightly lit */
constexpr auto F_LIT_DARK = 2; /* Darkened */
constexpr auto F_LIT_MAX = 3;
constexpr auto F_LIT_NS_BEGIN = 1; /* Nonstandard */

const std::map<int, DisplaySymbol> DEFAULT_SYMBOLS = { { F_LIT_STANDARD, {} }, { F_LIT_LITE, {} }, { F_LIT_DARK, {} } };

enum class TerrainAction {
    DESTROY = 1,
    NO_DROP = 2,
    CRASH_GLASS = 3,
    MAX,
};

class TerrainGenerationChange {
public:
    TerrainGenerationChange() = default;
    std::string result_tag{}; /*!< 生成時の変化先地形タグ */
    FEAT_IDX result{}; /*!< 生成時の変化先地形ID */
    int probability{}; /*!< 変化確率(%) */
};

/*!
 * @brief 地形状態変化指定構造体
 */
class TerrainState {
public:
    TerrainState() = default;
    TerrainCharacteristics action{}; /*!< 変化条件をFF_*のIDで指定 */
    std::string result_tag{}; /*!< 変化先ID */
    FEAT_IDX result{}; /*!< 変化先ID */
};

/*!
 * @brief 地形情報の構造体
 */
enum class TerrainTag;
class TerrainType {
public:
    TerrainType();
    FEAT_IDX idx{};
    std::string name; /*!< 地形名 */
    std::string text; /*!< 地形説明 */
    std::string tag; /*!< 地形特性タグ */

    /*!
     * @brief 地形特性タグ (enum class版)
     * @todo string型のtagが不要になったら削除する
     */
    TerrainTag tag_enum{};
    std::string mimic_tag;
    std::string destroyed_tag;
    FEAT_IDX mimic{}; /*!< 未確定時の外形地形ID / Feature to mimic */
    FEAT_IDX destroyed{}; /*!< *破壊*に巻き込まれた時の地形移行先(未実装？) / Default destroyed state */
    EnumClassFlagGroup<TerrainCharacteristics> flags{}; /*!< 地形の基本特性ビット配列 / Flags */
    int16_t priority{}; /*!< 縮小表示で省略する際の表示優先度 / Map priority */
    TerrainState state[MAX_FEAT_STATES]{}; /*!< TerrainState テーブル */
    TrapType trap_type = TrapType::NOT_TRAP; /*!< トラップ種別 */
    PatternTileType pattern_tile_type = PatternTileType::NOT_PATTERN; /*!< パターン種別 */
    StoreSaleType store_sale_type = StoreSaleType::NONE; /*!< 店舗種別 */
    BuildingType building_type = BuildingType::NONE; /*!< 施設種別 */
    TerrainConversionType conversion_type = TerrainConversionType::NONE; /*!< 変換地形種別 */
    int stream_index = -1; /*!< 川地形種別 */
    uint8_t door_power{}; /*!< 扉の強度 */
    uint8_t trap_power{}; /*!< 罠の解除難易度 */
    uint8_t tunnel_power{}; /*!< トンネル掘削難易度 */
    std::map<int, DisplaySymbol> symbol_definitions; //!< デフォルトの地形シンボル (色/文字).
    std::map<int, DisplaySymbol> symbol_configs; //!< 設定変更後の地形シンボル (色/文字).
    int change_priority;
    int hygiene = 0;

    // 時間経過による確率的地形変化
    int random_change_prob = 0; /*!< ランダム変化確率の逆数 (0=変化しない, 100なら1/100の確率) */
    std::string random_change_tag{}; /*!< ランダム変化先地形タグ */
    FEAT_IDX random_change = 0; /*!< ランダム変化先地形ID */

    Dice gold_drop{}; /*!< 財宝地形 (HAS_GOLD) を掘った際に生成する財宝数 (上流 #5393 相当) */
    std::vector<TerrainGenerationChange> generation_changes; /*!< 生成時の変化候補 (上流 #5393 相当) */

    static bool has(TerrainCharacteristics tc, TerrainAction ta);

    bool is_permanent_wall() const;
    bool can_damage_player() const;
    bool is_open() const;
    bool is_closed_door() const;
    bool has(TerrainCharacteristics tc) const;
    bool set_specific_type(uint8_t specific_type);

    void reset_lighting(bool is_config = true);

private:
    void reset_lighting_ascii(std::map<int, DisplaySymbol> &symbols);
    void reset_lighting_graphics(std::map<int, DisplaySymbol> &symbols);
};
