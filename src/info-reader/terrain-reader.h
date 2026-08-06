#pragma once

#include "system/angband.h"
#include <nlohmann/json.hpp>

errr parse_terrains_json_info(nlohmann::json &element);
