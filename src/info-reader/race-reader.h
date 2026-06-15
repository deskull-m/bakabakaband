#pragma once

#include "system/angband.h"
#include <nlohmann/json.hpp>
#include <string_view>

errr parse_monraces_info(nlohmann::json &element);
