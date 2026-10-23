#pragma once

#include "info-reader/parse-error-types.h"
#include "system/angband.h"
#include <nlohmann/json_fwd.hpp>
#include <string_view>

parse_error_type parse_creature_parties_info(nlohmann::json &party_data);
