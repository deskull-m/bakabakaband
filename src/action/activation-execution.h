#pragma once
/*!
 * @file activation-execution.h
 * @brief アイテムの発動実行ヘッダ
 */

#include "system/angband.h"

class CreatureEntity;
void exe_activate(CreatureEntity &creature, INVENTORY_IDX i_idx);
