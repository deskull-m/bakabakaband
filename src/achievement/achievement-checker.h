#pragma once

/*!
 * @file achievement-checker.h
 * @brief 実績判定処理
 * @date 2025/11/26
 * @author deskull
 */

#include "system/angband.h"

class PlayerType;
enum class MonraceId : int16_t;

/*!
 * @brief 実績チェッカー名前空間
 */
namespace AchievementChecker {

/*!
 * @brief ダンジョン進入時の実績チェック
 * @param player_ptr プレイヤー情報への参照ポインタ
 */
void check_dungeon_entry(PlayerType *player_ptr);

/*!
 * @brief 階層到達時の実績チェック
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param depth 現在の階層
 */
void check_depth_reached(PlayerType *player_ptr, DEPTH depth);

/*!
 * @brief モンスター撃破時の実績チェック
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param monrace_id 撃破したモンスターの種族ID
 */
void check_monster_kill(PlayerType *player_ptr, MonraceId monrace_id);

/*!
 * @brief レベルアップ時の実績チェック
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param new_level 新しいレベル
 */
void check_level_up(PlayerType *player_ptr, PLAYER_LEVEL new_level);

/*!
 * @brief アイテム入手時の実績チェック
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param is_artifact ★かどうか
 */
void check_item_acquisition(PlayerType *player_ptr, bool is_artifact);

/*!
 * @brief 死亡時の実績チェック
 * @param player_ptr プレイヤー情報への参照ポインタ
 */
void check_death(PlayerType *player_ptr);

/*!
 * @brief ゲームクリア時の実績チェック
 * @param player_ptr プレイヤー情報への参照ポインタ
 */
void check_game_win(PlayerType *player_ptr);

} // namespace AchievementChecker
