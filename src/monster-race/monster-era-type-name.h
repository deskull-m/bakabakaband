/*!
 * @brief モンスター文明ランク名称取得
 * @author Hourier
 * @date 2025/11/21
 */

#pragma once

#include "monster-race/race-era-flags.h"
#include <string>

std::string get_monster_era_type_name(MonsterEraType era);
std::string get_monster_era_type_tag(MonsterEraType era);
