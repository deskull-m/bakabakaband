#pragma once

/*!
 * @file achievement-checker.h
 * @brief 実績判定処理
 * @date 2025/11/26
 * @author deskull
 */

#include "system/angband.h"

class CreatureEntity;
enum class MonraceId : int16_t;

/*!
 * @brief 実績チェッカー名前空間
 */
namespace AchievementChecker {

/*!
 * @brief ダンジョン進入時の実績チェック
 * @param creature クリーチャーへの参照
 */
void check_dungeon_entry(CreatureEntity &creature);

/*!
 * @brief 階層到達時の実績チェック
 * @param creature クリーチャーへの参照
 * @param depth 現在の階層
 */
void check_depth_reached(CreatureEntity &creature, DEPTH depth);

/*!
 * @brief モンスター撃破時の実績チェック
 * @param creature クリーチャーへの参照
 * @param monrace_id 撃破したモンスターの種族ID
 */
void check_monster_kill(CreatureEntity &creature, MonraceId monrace_id);

/*!
 * @brief レベルアップ時の実績チェック
 * @param creature クリーチャーへの参照
 * @param new_level 新しいレベル
 */
void check_level_up(CreatureEntity &creature, PLAYER_LEVEL new_level);

/*!
 * @brief アイテム入手時の実績チェック
 * @param creature クリーチャーへの参照
 * @param is_artifact ★かどうか
 */
void check_item_acquisition(CreatureEntity &creature, bool is_artifact);

/*!
 * @brief 死亡時の実績チェック
 * @param creature クリーチャーへの参照
 */
void check_death(CreatureEntity &creature);

/*!
 * @brief ゲームクリア時の実績チェック
 * @param creature クリーチャーへの参照
 */
void check_game_win(CreatureEntity &creature);

} // namespace AchievementChecker
