#pragma once

#include "info-reader/parse-error-types.h"
#include "system/angband.h"
#include <nlohmann/json.hpp>
#include <string_view>

struct angband_header;

errr parse_creature_parties_info(nlohmann::json &party_data, angband_header *);
