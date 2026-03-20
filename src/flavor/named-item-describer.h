#pragma once

#include <string>

struct describe_option_type;
class CreatureEntity;
class ItemEntity;
std::string describe_named_item(CreatureEntity &creature, const ItemEntity &item, const describe_option_type &opt);
