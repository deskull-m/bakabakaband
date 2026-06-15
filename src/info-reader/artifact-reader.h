#pragma once

#include "system/angband.h"
#include <nlohmann/json.hpp>
#include <string_view>

errr parse_artifacts_info(nlohmann::json &element);
