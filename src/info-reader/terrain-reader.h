#pragma once

#include "info-reader/parse-error-types.h"
#include "system/angband.h"
#include <nlohmann/json_fwd.hpp>

ParseErrorType parse_terrains_json_info(nlohmann::json &element);
