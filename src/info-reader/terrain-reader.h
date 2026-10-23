#pragma once

#include "info-reader/parse-error-types.h"
#include "system/angband.h"
#include <nlohmann/json_fwd.hpp>

parse_error_type parse_terrains_json_info(nlohmann::json &element);
