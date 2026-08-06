#pragma once
/*!
 * @file scene-table-floor.h
 * @brief フロアの状況に応じたBGM設定処理ヘッダ
 */

#include "main/scene-table.h"
#include "system/angband.h"

class CreatureEntity;
int get_scene_floor_count();
void refresh_scene_floor(CreatureEntity &creature, scene_type_list &list, int start_index);
