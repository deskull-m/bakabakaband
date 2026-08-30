#pragma once

#include <stdint.h>
#include <string_view>

class CreatureEntity;
enum parse_error_type : int;
parse_error_type parse_fixed_map(CreatureEntity &creature, std::string_view name, int ymin, int xmin, int ymax, int xmax);
