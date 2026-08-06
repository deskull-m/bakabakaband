#pragma once
/*!
 * @file weapon-shield.h
 * @brief 手装備持ち替え処理ヘッダ
 */

#include "system/angband.h"

class CreatureEntity;
void verify_equip_slot(CreatureEntity &creature, INVENTORY_IDX i_idx);
