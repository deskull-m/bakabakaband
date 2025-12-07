/*!
 * @file achievement-definitions.cpp
 * @brief 実績定義データベースの実装
 * @date 2025/11/26
 * @author deskull
 */

#include "achievement/achievement-definitions.h"

using AD = AchievementDefinition;
using AT = AchievementType;
using AC = AchievementCategory;
using ADiff = AchievementDifficulty;

const std::map<AchievementType, std::shared_ptr<AchievementDefinition>> achievement_definitions = {
    // 探索系
    { AT::FIRST_DUNGEON_ENTRY,
        std::make_shared<AD>(
            AT::FIRST_DUNGEON_ENTRY,
            "初めてのダンジョン",
            "First Dungeon",
            "初めてダンジョンに足を踏み入れた",
            "Entered a dungeon for the first time",
            AC::EXPLORATION,
            ADiff::TRIVIAL,
            5) },

    { AT::REACH_DEPTH_10,
        std::make_shared<AD>(
            AT::REACH_DEPTH_10,
            "浅層探索者",
            "Shallow Explorer",
            "地下10階に到達した",
            "Reached dungeon level 10",
            AC::EXPLORATION,
            ADiff::EASY,
            10) },

    { AT::REACH_DEPTH_50,
        std::make_shared<AD>(
            AT::REACH_DEPTH_50,
            "中層探索者",
            "Intermediate Explorer",
            "地下50階に到達した",
            "Reached dungeon level 50",
            AC::EXPLORATION,
            ADiff::NORMAL,
            25) },

    { AT::REACH_DEPTH_100,
        std::make_shared<AD>(
            AT::REACH_DEPTH_100,
            "深淵の探索者",
            "Deep Explorer",
            "地下100階に到達した",
            "Reached dungeon level 100",
            AC::EXPLORATION,
            ADiff::HARD,
            50) },

    // 戦闘系
    { AT::FIRST_KILL,
        std::make_shared<AD>(
            AT::FIRST_KILL,
            "初めての勝利",
            "First Victory",
            "初めてモンスターを倒した",
            "Defeated your first monster",
            AC::COMBAT,
            ADiff::TRIVIAL,
            5) },

    { AT::KILL_100_MONSTERS,
        std::make_shared<AD>(
            AT::KILL_100_MONSTERS,
            "駆け出しハンター",
            "Novice Hunter",
            "100体のモンスターを倒した",
            "Defeated 100 monsters",
            AC::COMBAT,
            ADiff::EASY,
            10) },

    { AT::KILL_1000_MONSTERS,
        std::make_shared<AD>(
            AT::KILL_1000_MONSTERS,
            "ベテランハンター",
            "Veteran Hunter",
            "1000体のモンスターを倒した",
            "Defeated 1000 monsters",
            AC::COMBAT,
            ADiff::NORMAL,
            30) },

    { AT::KILL_FIRST_UNIQUE,
        std::make_shared<AD>(
            AT::KILL_FIRST_UNIQUE,
            "伝説への第一歩",
            "First Legend",
            "初めてユニークモンスターを倒した",
            "Defeated your first unique monster",
            AC::COMBAT,
            ADiff::EASY,
            15) },

    // レベル系
    { AT::REACH_LEVEL_10,
        std::make_shared<AD>(
            AT::REACH_LEVEL_10,
            "駆け出し冒険者",
            "Beginner Adventurer",
            "レベル10に到達した",
            "Reached level 10",
            AC::CHARACTER,
            ADiff::TRIVIAL,
            5) },

    { AT::REACH_LEVEL_25,
        std::make_shared<AD>(
            AT::REACH_LEVEL_25,
            "熟練冒険者",
            "Skilled Adventurer",
            "レベル25に到達した",
            "Reached level 25",
            AC::CHARACTER,
            ADiff::EASY,
            15) },

    { AT::REACH_LEVEL_50,
        std::make_shared<AD>(
            AT::REACH_LEVEL_50,
            "伝説の冒険者",
            "Legendary Adventurer",
            "レベル50に到達した",
            "Reached level 50",
            AC::CHARACTER,
            ADiff::HARD,
            50) },

    // アイテム系
    { AT::FIRST_ARTIFACT,
        std::make_shared<AD>(
            AT::FIRST_ARTIFACT,
            "トレジャーハンター",
            "Treasure Hunter",
            "初めて★を入手した",
            "Found your first artifact",
            AC::COLLECTION,
            ADiff::EASY,
            10) },

    { AT::COLLECT_10_ARTIFACTS,
        std::make_shared<AD>(
            AT::COLLECT_10_ARTIFACTS,
            "コレクター",
            "Collector",
            "10個の★を入手した",
            "Collected 10 artifacts",
            AC::COLLECTION,
            ADiff::NORMAL,
            25) },

    // 特殊系
    { AT::WIN_GAME,
        std::make_shared<AD>(
            AT::WIN_GAME,
            "世界の救世主",
            "Savior of the World",
            "ゲームをクリアした",
            "Won the game",
            AC::SPECIAL,
            ADiff::VERY_HARD,
            100) },

    { AT::FIRST_DEATH,
        std::make_shared<AD>(
            AT::FIRST_DEATH,
            "永遠の眠り",
            "Eternal Sleep",
            "初めて死亡した",
            "Died for the first time",
            AC::SPECIAL,
            ADiff::TRIVIAL,
            5) },
};

const std::shared_ptr<AchievementDefinition> &get_achievement_definition(AchievementType type)
{
    static std::shared_ptr<AchievementDefinition> null_def;

    auto it = achievement_definitions.find(type);
    if (it == achievement_definitions.end()) {
        return null_def;
    }

    return it->second;
}
