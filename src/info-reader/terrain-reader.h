#pragma once

#include "system/angband.h"
#include <nlohmann/json.hpp>

struct angband_header;
errr parse_terrains_json_info(nlohmann::json &element, angband_header *head);
