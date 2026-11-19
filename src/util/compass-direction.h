/*!
 * @brief 16 方位の方角名を扱うユーティリティ
 */

#pragma once

#include "util/point-2d.h"
#include <string_view>

int calc_compass_direction_index(const Pos2DVec &vec);
std::string_view describe_compass_direction(const Pos2DVec &vec);
