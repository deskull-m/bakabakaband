/*!
 * @file achievement-checker.cpp
 * @brief 実績判定処理の実装
 * @date 2025/11/26
 * @author deskull
 */

#include "achievement/achievement-checker.h"
#include "achievement/achievement-tracker.h"
#include "achievement/achievement-types.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"

namespace AchievementChecker {

void check_dungeon_entry(PlayerType *player_ptr)
{
    auto &tracker = AchievementTracker::get_instance();
    tracker.unlock_achievement(player_ptr, AchievementType::FIRST_DUNGEON_ENTRY);
}

void check_depth_reached(PlayerType *player_ptr, DEPTH depth)
{
    auto &tracker = AchievementTracker::get_instance();

    if (depth >= 10) {
        tracker.unlock_achievement(player_ptr, AchievementType::REACH_DEPTH_10);
    }
    if (depth >= 50) {
        tracker.unlock_achievement(player_ptr, AchievementType::REACH_DEPTH_50);
    }
    if (depth >= 100) {
        tracker.unlock_achievement(player_ptr, AchievementType::REACH_DEPTH_100);
    }
}

void check_monster_kill(PlayerType *player_ptr, MonraceId monrace_id)
{
    auto &tracker = AchievementTracker::get_instance();

    // 初めての撃破
    tracker.unlock_achievement(player_ptr, AchievementType::FIRST_KILL);

    // 総撃破数のカウント（実際のカウントはplayer_ptrから取得すべき）
    // TODO: player_ptrに総撃破数フィールドを追加

    // ユニーク撃破チェック
    const auto &monrace = MonraceList::get_instance().get_monrace(monrace_id);
    if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
        tracker.unlock_achievement(player_ptr, AchievementType::KILL_FIRST_UNIQUE);
        // TODO: ユニーク撃破数のカウント
    }
}

void check_level_up(PlayerType *player_ptr, PLAYER_LEVEL new_level)
{
    auto &tracker = AchievementTracker::get_instance();

    if (new_level >= 10) {
        tracker.unlock_achievement(player_ptr, AchievementType::REACH_LEVEL_10);
    }
    if (new_level >= 25) {
        tracker.unlock_achievement(player_ptr, AchievementType::REACH_LEVEL_25);
    }
    if (new_level >= 50) {
        tracker.unlock_achievement(player_ptr, AchievementType::REACH_LEVEL_50);
    }
}

void check_item_acquisition(PlayerType *player_ptr, bool is_artifact)
{
    auto &tracker = AchievementTracker::get_instance();

    if (is_artifact) {
        tracker.unlock_achievement(player_ptr, AchievementType::FIRST_ARTIFACT);
        // TODO: ★入手数のカウント
    }
}

void check_death(PlayerType *player_ptr)
{
    auto &tracker = AchievementTracker::get_instance();
    tracker.unlock_achievement(player_ptr, AchievementType::FIRST_DEATH);
    // TODO: 死亡回数のカウント
}

void check_game_win(PlayerType *player_ptr)
{
    auto &tracker = AchievementTracker::get_instance();
    tracker.unlock_achievement(player_ptr, AchievementType::WIN_GAME);

    // TODO: アイアンマンモードチェック
    // TODO: スピードランチェック
}

} // namespace AchievementChecker
