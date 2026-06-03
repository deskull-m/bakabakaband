#pragma once
/*!
 * @file creature-common-loader.h
 * @brief プレイヤー・モンスター共通 (CreatureEntity) のロード処理
 * @details
 * wr_creature_common() と対称の読み込み。セーブデータバージョン 50 以降で
 * 使用する。CreatureEntity の基底フィールドを復元する。
 */

class CreatureEntity;

/*!
 * @brief CreatureEntity の共通基底フィールドをセーブデータから読み込む
 * @param creature 復元対象クリーチャー
 */
void rd_creature_common(CreatureEntity &creature);
