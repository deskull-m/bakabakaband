#pragma once

/*!
 * @file achievement-types.h
 * @brief ゲーム実績の種別定義
 * @date 2025/11/26
 * @author deskull
 */

#include "system/angband.h"
#include <cstdint>

/*!
 * @brief 実績の種類
 */
enum class AchievementType : int16_t {
    NONE = 0,

    // 探索系
    FIRST_DUNGEON_ENTRY = 1, /*!< 初めてダンジョンに入る */
    REACH_DEPTH_10 = 2, /*!< 地下10階到達 */
    REACH_DEPTH_50 = 3, /*!< 地下50階到達 */
    REACH_DEPTH_100 = 4, /*!< 地下100階到達 */
    VISIT_ALL_TOWNS = 5, /*!< 全ての町を訪問 */

    // 戦闘系
    FIRST_KILL = 10, /*!< 初めてモンスターを倒す */
    KILL_100_MONSTERS = 11, /*!< 100体のモンスターを倒す */
    KILL_1000_MONSTERS = 12, /*!< 1000体のモンスターを倒す */
    KILL_FIRST_UNIQUE = 13, /*!< 初めてユニークを倒す */
    KILL_10_UNIQUES = 14, /*!< 10体のユニークを倒す */

    // レベル系
    REACH_LEVEL_10 = 20, /*!< レベル10到達 */
    REACH_LEVEL_25 = 21, /*!< レベル25到達 */
    REACH_LEVEL_50 = 22, /*!< レベル50到達 */
    REACH_MAX_LEVEL = 23, /*!< 最大レベル到達 */

    // アイテム系
    FIRST_ARTIFACT = 30, /*!< 初めて★を入手 */
    COLLECT_10_ARTIFACTS = 31, /*!< 10個の★を入手 */
    COLLECT_50_ARTIFACTS = 32, /*!< 50個の★を入手 */
    FIRST_EGO_ITEM = 33, /*!< 初めてエゴアイテムを入手 */

    // 死亡系
    FIRST_DEATH = 40, /*!< 初めて死亡する */
    DIE_10_TIMES = 41, /*!< 10回死亡する */
    DIE_TO_UNIQUE = 42, /*!< ユニークに殺される */

    // クエスト系
    COMPLETE_FIRST_QUEST = 50, /*!< 初めてクエストをクリア */
    COMPLETE_10_QUESTS = 51, /*!< 10個のクエストをクリア */

    // 特殊系
    WIN_GAME = 100, /*!< ゲームクリア */
    WIN_GAME_IRONMAN = 101, /*!< アイアンマンモードでクリア */
    SPEEDRUN_WIN = 102, /*!< 短時間でクリア */

    MAX,
};

/*!
 * @brief 実績の難易度
 */
enum class AchievementDifficulty : uint8_t {
    TRIVIAL = 0, /*!< 自然に達成される */
    EASY = 1, /*!< 簡単 */
    NORMAL = 2, /*!< 普通 */
    HARD = 3, /*!< 難しい */
    VERY_HARD = 4, /*!< 非常に難しい */
};

/*!
 * @brief 実績のカテゴリ
 */
enum class AchievementCategory : uint8_t {
    EXPLORATION = 0, /*!< 探索 */
    COMBAT = 1, /*!< 戦闘 */
    CHARACTER = 2, /*!< キャラクター成長 */
    COLLECTION = 3, /*!< 収集 */
    QUEST = 4, /*!< クエスト */
    SPECIAL = 5, /*!< 特殊 */
};
