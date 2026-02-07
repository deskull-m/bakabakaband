#pragma once
/*!
 * @file open-close-execution.h
 * @brief 扉や箱を開ける処理のヘッダ
 */

#include "system/angband.h"
#include "util/point-2d.h"

class Direction;
class CreatureEntity;
bool exe_open(CreatureEntity &creature, POSITION y, POSITION x);
bool exe_close(CreatureEntity &creature, const Pos2D &pos);
bool easy_open_door(CreatureEntity &creature, const Pos2D &pos);
bool exe_disarm(CreatureEntity &creature, POSITION y, POSITION x, const Direction &dir);
bool exe_disarm_chest(CreatureEntity &creature, POSITION y, POSITION x, OBJECT_IDX o_idx);
bool exe_bash(CreatureEntity &creature, POSITION y, POSITION x, const Direction &dir);
