#pragma once

#include "system/angband.h"
#include <nlohmann/json.hpp>
#include <string_view>

errr parse_class_magics_info(nlohmann::json &class_data);
