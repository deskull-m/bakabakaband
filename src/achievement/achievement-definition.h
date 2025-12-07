#pragma once

/*!
 * @file achievement-definition.h
 * @brief 実績定義構造体
 * @date 2025/11/26
 * @author deskull
 */

#include "achievement/achievement-types.h"
#include <string>

/*!
 * @brief 実績定義構造体
 */
struct AchievementDefinition {
    AchievementType type; /*!< 実績ID */
    std::string name_ja; /*!< 日本語名 */
    std::string name_en; /*!< 英語名 */
    std::string description_ja; /*!< 日本語説明 */
    std::string description_en; /*!< 英語説明 */
    AchievementCategory category; /*!< カテゴリ */
    AchievementDifficulty difficulty; /*!< 難易度 */
    int32_t points; /*!< 獲得ポイント */
    bool is_hidden; /*!< 隠し実績かどうか */

    /*!
     * @brief コンストラクタ
     */
    AchievementDefinition(
        AchievementType type_,
        std::string name_ja_,
        std::string name_en_,
        std::string description_ja_,
        std::string description_en_,
        AchievementCategory category_,
        AchievementDifficulty difficulty_,
        int32_t points_,
        bool is_hidden_ = false)
        : type(type_)
        , name_ja(std::move(name_ja_))
        , name_en(std::move(name_en_))
        , description_ja(std::move(description_ja_))
        , description_en(std::move(description_en_))
        , category(category_)
        , difficulty(difficulty_)
        , points(points_)
        , is_hidden(is_hidden_)
    {
    }
};
