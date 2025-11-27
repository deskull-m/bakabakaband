#pragma once

/*!
 * @file achievement-definitions.h
 * @brief 実績定義データベース
 * @date 2025/11/26
 * @author deskull
 */

#include "achievement/achievement-definition.h"
#include "achievement/achievement-types.h"
#include <map>
#include <memory>

/*!
 * @brief 実績定義データベース
 */
extern const std::map<AchievementType, std::shared_ptr<AchievementDefinition>> achievement_definitions;

/*!
 * @brief 実績定義を取得
 * @param type 実績ID
 * @return 実績定義へのポインタ（見つからない場合はnullptr）
 */
const std::shared_ptr<AchievementDefinition> &get_achievement_definition(AchievementType type);
