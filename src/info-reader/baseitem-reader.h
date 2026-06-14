#pragma once

#include "system/angband.h"
#include <nlohmann/json.hpp>
#include <string_view>

struct angband_header;
errr parse_baseitems_info(nlohmann::json &element, angband_header *head);
