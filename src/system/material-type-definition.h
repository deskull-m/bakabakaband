#pragma once
/*!
 * @file material-type-definition.h
 * @brief クリーチャーの「材質」(副種族) 定義
 * @details
 * 材質は CreatureEntity の副種族として複数同時に保持でき、それぞれが
 * 能力値修正 (6 能力値) と AC 修正を持つ。鉄や金などモンスターの構成材質を
 * 種族とは独立した軸として表現する。能力値修正は表示単位 (1 = +1.0) で
 * 記述し、適用時に内部 10 単位へ変換する。AC 修正は素の AC 値への加算。
 * gold_drop_percent は当該材質で構成されたモンスターが落とす金銭額の倍率
 * (% 単位、100 = 等倍)。貴金属系ほど高く、紙や糞は低い。
 */

#include "locale/localized-string.h"
#include "player-ability/player-ability-types.h"
#include <array>
#include <cstdint>

enum class CreatureMaterialType : int {
    FLESH = 0, //!< 肉
    WOODEN = 1, //!< 木
    PAPER = 2, //!< 紙
    STONE = 3, //!< 石
    IRON = 4, //!< 鉄
    COPPER = 5, //!< 銅
    SILVER = 6, //!< 銀
    GOLD = 7, //!< 金
    MITHRIL = 8, //!< ミスリル
    ADAMANTITE = 9, //!< アダマンタイト
    DARKSTEEL = 10, //!< 漆黒鋼
    WARPSTONE = 11, //!< 歪魂石
    FECES = 12, //!< 糞
    MAX,
};

constexpr auto MATERIAL_TYPE_MAX = static_cast<int>(CreatureMaterialType::MAX);

struct MaterialDefinition {
    LocalizedString name; //!< 材質名
    std::array<int, A_MAX> stat_modifiers; //!< 能力値修正 (表示単位、1 = +1.0)
    int ac_modifier; //!< AC 修正 (素の AC への加算)
    int gold_drop_percent; //!< 金銭ドロップ額の倍率 (% 単位、100 = 等倍)
};

/*!
 * @brief 材質定義テーブルを取得する
 * @param material 材質種別
 * @return 当該材質の定義への参照
 */
inline const MaterialDefinition &get_material_definition(CreatureMaterialType material)
{
    // 添字は CreatureMaterialType の値と一致させること。
    static const std::array<MaterialDefinition, MATERIAL_TYPE_MAX> definitions = { {
        // name,                          STR INT WIS DEX CON CHR    AC  gold%
        { { "肉", "Flesh" }, { 0, 0, 0, 0, 0, 0 }, 0, 100 },
        { { "木", "Wood" }, { 1, 0, 0, -1, 1, 0 }, 3, 100 },
        { { "紙", "Paper" }, { -3, 0, 0, 2, -3, 0 }, -3, 80 },
        { { "石", "Stone" }, { 3, -2, 0, -3, 4, 0 }, 15, 110 },
        { { "鉄", "Iron" }, { 4, -2, 0, -3, 5, 0 }, 25, 130 },
        { { "銅", "Copper" }, { 3, -1, 0, -2, 4, 0 }, 18, 160 },
        { { "銀", "Silver" }, { 3, 1, 0, -1, 3, 2 }, 20, 220 },
        { { "金", "Gold" }, { 2, 0, 0, -4, 3, 4 }, 22, 500 },
        { { "ミスリル", "Mithril" }, { 5, 1, 0, 0, 5, 2 }, 30, 350 },
        { { "アダマンタイト", "Adamantite" }, { 7, -1, 0, -2, 7, 0 }, 40, 280 },
        { { "漆黒鋼", "Darksteel" }, { 6, 0, 0, -1, 6, 0 }, 35, 200 },
        { { "歪魂石", "Warpstone" }, { 4, 3, -2, 0, 3, -2 }, 28, 180 },
        { { "糞", "Feces" }, { -2, -3, -2, -2, -2, -5 }, -5, 50 },
    } };

    return definitions[static_cast<int>(material)];
}
