/*!
 * @file achievement-tracker.cpp
 * @brief 実績追跡システムの実装
 * @date 2025/11/26
 * @author deskull
 */

#include "achievement/achievement-tracker.h"
#include "achievement/achievement-definitions.h"
#include "core/stuff-handler.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "view/display-messages.h"
#include "world/world.h"

AchievementTracker::AchievementTracker()
    : total_points_(0)
{
}

AchievementTracker &AchievementTracker::get_instance()
{
    static AchievementTracker instance;
    return instance;
}

bool AchievementTracker::unlock_achievement(PlayerType *player_ptr, AchievementType achievement)
{
    if (achievement == AchievementType::NONE || achievement >= AchievementType::MAX) {
        return false;
    }

    // 既に解除済みの場合は何もしない
    if (is_unlocked(achievement)) {
        return false;
    }

    // 実績を解除
    auto &progress = progress_map_[achievement];
    progress.unlocked = true;
    progress.unlock_time = AngbandWorld::get_instance().game_turn;

    unlocked_set_.insert(achievement);

    // ポイントを加算
    const auto &def = get_achievement_definition(achievement);
    if (def) {
        total_points_ += def->points;
    }

    // 通知を表示
    show_achievement_notification(player_ptr, achievement);

    return true;
}

bool AchievementTracker::is_unlocked(AchievementType achievement) const
{
    return unlocked_set_.count(achievement) > 0;
}

void AchievementTracker::update_progress([[maybe_unused]] PlayerType *player_ptr, AchievementType achievement, int32_t value)
{
    if (achievement == AchievementType::NONE || achievement >= AchievementType::MAX) {
        return;
    }

    auto &progress = progress_map_[achievement];
    progress.current_value = value;

    // 進捗が一定値に達したら解除（実績ごとの条件判定は別途実装）
}

int32_t AchievementTracker::get_progress(AchievementType achievement) const
{
    auto it = progress_map_.find(achievement);
    if (it == progress_map_.end()) {
        return 0;
    }
    return it->second.current_value;
}

size_t AchievementTracker::get_unlocked_count() const
{
    return unlocked_set_.size();
}

int32_t AchievementTracker::get_total_points() const
{
    return total_points_;
}

void AchievementTracker::reset()
{
    progress_map_.clear();
    unlocked_set_.clear();
    total_points_ = 0;
}

bool AchievementTracker::save_achievements([[maybe_unused]] PlayerType *player_ptr)
{
    // TODO: セーブ処理の実装
    // セーブファイルに実績データを書き込む
    return true;
}

bool AchievementTracker::load_achievements([[maybe_unused]] PlayerType *player_ptr)
{
    // TODO: ロード処理の実装
    // セーブファイルから実績データを読み込む
    return true;
}

const std::set<AchievementType> &AchievementTracker::get_unlocked_achievements() const
{
    return unlocked_set_;
}

void AchievementTracker::show_achievement_notification([[maybe_unused]] PlayerType *player_ptr, AchievementType achievement)
{
    const auto &def = get_achievement_definition(achievement);
    if (!def) {
        return;
    }

#ifdef JP
    const auto &name = def->name_ja;
    const auto &description = def->description_ja;
#else
    const auto &name = def->name_en;
    const auto &description = def->description_en;
#endif

    msg_print("*** 実績解除! ***");
    msg_format("『%s』", name.c_str());
    msg_format("%s", description.c_str());
    msg_format("(%dポイント獲得)", def->points);
}
