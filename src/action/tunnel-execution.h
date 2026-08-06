#pragma once
/*!
 * @file tunnel-execution.h
 * @brief 掘削処理ヘッダ
 */

#include "system/angband.h"

class CreatureEntity;
bool exe_tunnel(CreatureEntity &creature, POSITION y, POSITION x);
