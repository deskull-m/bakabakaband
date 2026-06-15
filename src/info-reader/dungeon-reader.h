#pragma once

#include "system/angband.h"
#include <nlohmann/json.hpp>
#include <string_view>

errr parse_dungeons_info(std::string_view buf);
errr parse_dungeons_info_json(nlohmann::json &dungeon_data);
