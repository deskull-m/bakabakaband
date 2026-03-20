#pragma once

#include "system/angband.h"
#include <string_view>

class CreatureEntity;
class ItemEntity;
std::string describe_flavor(CreatureEntity &creature, const ItemEntity &item, const BIT_FLAGS mode, const size_t max_length = std::string_view::npos);
