#pragma once

#include "object/tval-types.h"
#include "system/angband.h"
#include <tl/optional.hpp>

class CreatureEntity;
class ItemTester;
tl::optional<short> get_item_floor(CreatureEntity &creature, std::string_view pmt, std::string_view str, BIT_FLAGS mode, const ItemTester &item_tester);
