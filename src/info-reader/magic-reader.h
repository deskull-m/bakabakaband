#pragma once

#include "system/angband.h"
#include <nlohmann/json.hpp>
#include <string_view>

struct angband_header;
errr parse_class_magics_info(nlohmann::json &class_data, angband_header *head);
