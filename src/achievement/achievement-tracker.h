#pragma once

/*!
 * @file achievement-tracker.h
 * @brief 実績追跡システム
 * @date 2025/11/26
 * @author deskull
 */

#include "achievement/achievement-types.h"
#include <cstdint>
#include <map>
#include <set>
#include <string>

class PlayerType;

/*!
 * @brief 実績の進捗データ
 */
struct AchievementProgress {
    bool unlocked; /*!< 解除済みかどうか */
    int32_t current_value; /*!< 現在の進捗値 */
    int32_t unlock_time; /*!< 解除時刻(ゲームターン) */

    AchievementProgress()
        : unlocked(false)
        , current_value(0)
        , unlock_time(0)
    {
    }
};

/*!
 * @brief 実績追跡クラス
 */
class AchievementTracker {
public:
    AchievementTracker();
    ~AchievementTracker() = default;

    // シングルトンアクセス
    static AchievementTracker &get_instance();

    /*!
     * @brief 実績を解除する
     * @param player_ptr プレイヤー情報への参照ポインタ
     * @param achievement 実績ID
     * @return 新規解除された場合true
     */
    bool unlock_achievement(PlayerType *player_ptr, AchievementType achievement);

    /*!
     * @brief 実績が解除済みか確認
     * @param achievement 実績ID
     * @return 解除済みならtrue
     */
    bool is_unlocked(AchievementType achievement) const;

    /*!
     * @brief 実績の進捗を更新
     * @param player_ptr プレイヤー情報への参照ポインタ
     * @param achievement 実績ID
     * @param value 進捗値
     */
    void update_progress(PlayerType *player_ptr, AchievementType achievement, int32_t value);

    /*!
     * @brief 実績の進捗を取得
     * @param achievement 実績ID
     * @return 進捗値
     */
    int32_t get_progress(AchievementType achievement) const;

    /*!
     * @brief 解除済み実績数を取得
     * @return 解除済み実績数
     */
    size_t get_unlocked_count() const;

    /*!
     * @brief 合計ポイントを取得
     * @return 合計ポイント
     */
    int32_t get_total_points() const;

    /*!
     * @brief データをリセット
     */
    void reset();

    /*!
     * @brief データをセーブ
     * @param player_ptr プレイヤー情報への参照ポインタ
     * @return 成功したらtrue
     */
    bool save_achievements(PlayerType *player_ptr);

    /*!
     * @brief データをロード
     * @param player_ptr プレイヤー情報への参照ポインタ
     * @return 成功したらtrue
     */
    bool load_achievements(PlayerType *player_ptr);

    /*!
     * @brief 解除済み実績のセットを取得
     * @return 解除済み実績のセット
     */
    const std::set<AchievementType> &get_unlocked_achievements() const;

private:
    std::map<AchievementType, AchievementProgress> progress_map_; /*!< 進捗マップ */
    std::set<AchievementType> unlocked_set_; /*!< 解除済みセット */
    int32_t total_points_; /*!< 合計ポイント */

    /*!
     * @brief 実績解除通知を表示
     * @param player_ptr プレイヤー情報への参照ポインタ
     * @param achievement 実績ID
     */
    void show_achievement_notification(PlayerType *player_ptr, AchievementType achievement);
};
