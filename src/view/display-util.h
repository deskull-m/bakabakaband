#pragma once

#include "system/angband.h"
#include <string>
#include <string_view>

class CreatureEntity;

void display_player_one_line(int entry, std::string_view val, TERM_COLOR attr);
int display_wrap_around(std::string_view sv, size_t width, int start_row, int col);

/*!
 * @brief 装備部位ごとに 1 列を並べる表 (特性フラグ一覧・能力修正表) の列見出しを生成する
 * @param creature クリーチャーへの参照
 * @param weapon_slots_only 武器スロットのみを対象とするなら true (DP_WP 相当)
 * @return 各装備列の見出し文字 + 末尾にクリーチャー自身を表す '@'
 * @details 体構造的に存在しない部位は列ごと消すため、見出しもそれに合わせて生成する。
 *          文字は装備一覧と同じくスロット由来 (INVEN_MAIN_HAND が 'a') なので、
 *          部位を消しても残った列の文字は変わらない (例: 四足獣なら "fhj@")。
 *          列を並べる側 (display_player_equippy() や各表の描画) は
 *          CreatureEntity::should_display_equipment_slot() で同じ条件に揃えること。
 */
std::string build_equipment_column_labels(CreatureEntity &creature, bool weapon_slots_only = false);
