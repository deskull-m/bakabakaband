#pragma once

#include "external-lib/include-json.h"
#include "system/angband.h"

struct angband_header;
errr parse_terrains_json_info(nlohmann::json &element, angband_header *head);
