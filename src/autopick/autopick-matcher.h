#pragma once

#include "system/angband.h"
#include <string_view>

struct autopick_type;
class CreatureEntity;
class ItemEntity;
bool is_autopick_match(CreatureEntity &creature, const ItemEntity *o_ptr, const autopick_type &entry, std::string_view item_name);
