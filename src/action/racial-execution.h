#pragma once
/*!
 * @file racial-execution.h
 * @brief レイシャルパワー実行処理ヘッダ
 */

#include "system/angband.h"

enum racial_level_check_result {
    RACIAL_SUCCESS = 1,
    RACIAL_FAILURE = -1,
    RACIAL_CANCEL = 0,
};

struct rpi_type;
class CreatureEntity;
PERCENTAGE racial_chance(CreatureEntity &creature, rpi_type *rpi_ptr);
racial_level_check_result check_racial_level(CreatureEntity &creature, rpi_type *rpi_ptr);
bool exe_racial_power(CreatureEntity &creature, const int32_t command);
