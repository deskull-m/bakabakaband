#pragma once

class CreatureEntity;

/*!
 * @brief c コマンド第 6 ページ「装備＆所持品」を描画する
 * @param creature 表示対象クリーチャー (プレイヤー / モンスター共用)
 */
void display_player_inventory_page(CreatureEntity &creature);
