/*!
 * @file monster-birth.h
 * @brief モンスターをプレイヤーとしてゲーム開始する処理
 */

#pragma once

class CreatureEntity;

/*!
 * @brief ゲーム開始時にモンスターをプレイヤーとして生成する
 * @param creature 設定対象 (PlayerType::get_instance())
 * @return モンスター開始が完了したら true、キャンセル/失敗時 false
 * @details ウィザードモードの n コマンド相当の UI で MonraceId を選択し、
 *          そのモンスターをプレイヤーキャラクターとして初期化する。
 *          失敗時は呼び出し元で通常のキャラクター作成にフォールバックする想定。
 */
bool player_birth_as_monster(CreatureEntity &creature);
