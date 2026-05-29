/*!
 * @file monster-drop-generator.h
 * @brief モンスター生成時にドロップ品を所持品として生成する処理
 */

#pragma once

class CreatureEntity;

/*!
 * @brief モンスターの一般ドロップ品 (drop_flags に基づくアイテム/金) を
 *        生成し、そのモンスターの所持品 (inventory) に追加する。
 * @param player プレイヤーへの参照 (生成基準・フロア取得用)
 * @param monster ドロップ品を持たせる対象モンスター
 * @details 従来は monster_death() 時に生成・床散布していた一般ドロップを、
 *          モンスター生成時に所持品として前生成する方式へ移行したもの。
 *          死亡時は drop_all_inventory() により所持品ごと床へ放出される。
 *          固定アーティファクト・クエスト品・死体ドロップ等の特殊処理は
 *          引き続き死亡時に行う。
 */
void generate_monster_drop_items(CreatureEntity &player, CreatureEntity &monster);
