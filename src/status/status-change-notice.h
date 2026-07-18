#pragma once

class CreatureEntity;

/*!
 * @brief 一時ステータス setter 末尾の共通後処理 (再描画 → 視認可能な変化なら disturb/再計算/handle_stuff)
 * @param creature 対象クリーチャーへの参照
 * @param notice 観測可能な変化があったか (false なら再描画のみ行い false を返す)
 * @return notice が true のとき true、false のとき false
 * @details 多数の一時ステータス setter に byte 一致で重複していた末尾
 *          (TIMED_EFFECT 再描画 → notice なしなら return false → disturb →
 *          BONUS 再計算 → handle_stuff → return true) を集約した共通処理。挙動不変。
 */
bool notice_bonus_status_change(CreatureEntity &creature, bool notice);
