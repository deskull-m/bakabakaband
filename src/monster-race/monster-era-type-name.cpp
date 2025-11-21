/*!
 * @brief モンスター文明ランク名称取得実装
 * @author Hourier
 * @date 2025/11/21
 */

#include "monster-race/monster-era-type-name.h"

/*!
 * @brief モンスター文明ランクの名称を取得する
 * @param era 文明ランク
 * @return 文明ランクの名称
 */
std::string get_monster_era_type_name(MonsterEraType era)
{
    switch (era) {
    case MonsterEraType::PREHISTORIC:
        return "先史時代級";
    case MonsterEraType::ANCIENT:
        return "古代級";
    case MonsterEraType::MEDIEVAL:
        return "中世級";
    case MonsterEraType::EARLY_MODERN:
        return "近代級";
    case MonsterEraType::MODERN:
        return "現代級";
    case MonsterEraType::INFORMATION_AGE:
        return "情報化時代級";
    case MonsterEraType::NANOTECH:
        return "ナノテク級";
    default:
        return "不明";
    }
}

/*!
 * @brief モンスター文明ランクのタグ名を取得する
 * @param era 文明ランク
 * @return 文明ランクのタグ名
 */
std::string get_monster_era_type_tag(MonsterEraType era)
{
    switch (era) {
    case MonsterEraType::PREHISTORIC:
        return "PREHISTORIC";
    case MonsterEraType::ANCIENT:
        return "ANCIENT";
    case MonsterEraType::MEDIEVAL:
        return "MEDIEVAL";
    case MonsterEraType::EARLY_MODERN:
        return "EARLY_MODERN";
    case MonsterEraType::MODERN:
        return "MODERN";
    case MonsterEraType::INFORMATION_AGE:
        return "INFORMATION_AGE";
    case MonsterEraType::NANOTECH:
        return "NANOTECH";
    default:
        return "UNKNOWN";
    }
}
