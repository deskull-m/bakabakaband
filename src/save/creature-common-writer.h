#pragma once
/*!
 * @file creature-common-writer.h
 * @brief プレイヤー・モンスター共通 (CreatureEntity) のセーブ処理
 * @details
 * 旧 PlayerType / 旧 MonsterEntity のセーブ処理統合の基盤 (フェーズ 1)。
 * CreatureEntity の基底フィールド (HP・座標・速度・時限効果・材質等) を
 * プレイヤー様式の明示的フォーマットで 1 箇所に書き出す。セーブデータ
 * バージョン 50 以降で使用する。現状はモンスター経路が利用し、プレイヤー
 * 経路の移行は後続フェーズで行う。
 */

class CreatureEntity;

/*!
 * @brief CreatureEntity の共通基底フィールドをセーブデータに書き込む
 * @param creature 書き込み対象クリーチャー
 */
void wr_creature_common(const CreatureEntity &creature);
