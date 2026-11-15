/*!
 * @brief モンスター文明ランク名称取得実装
 * @author Hourier
 * @date 2025/11/21
 */

#include "monster-race/monster-era-type-name.h"
#include "locale/language-switcher.h"

/*!
 * @brief モンスター文明ランクの名称を取得する
 * @param era 文明ランク
 * @return 文明ランクの名称
 */
std::string get_monster_era_type_name(MonsterEraType era)
{
    switch (era) {
    case MonsterEraType::PREHISTORIC:
        return _("先史時代級", "prehistoric-era");
    case MonsterEraType::ANCIENT:
        return _("古代級", "ancient-era");
    case MonsterEraType::MEDIEVAL:
        return _("中世級", "medieval-era");
    case MonsterEraType::EARLY_MODERN:
        return _("近代級", "early-modern-era");
    case MonsterEraType::MODERN:
        return _("現代級", "modern-era");
    case MonsterEraType::INFORMATION_AGE:
        return _("情報化時代級", "information-age");
    case MonsterEraType::NANOTECH:
        return _("ナノテク級", "nanotech-era");
    default:
        return _("不明", "unknown-era");
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
