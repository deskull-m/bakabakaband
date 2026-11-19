#pragma once

#include <string>
#include <utility>

class CreatureEntity;
class ItemEntity;
struct describe_option_type;
std::pair<std::string, std::string> switch_tval_description(const CreatureEntity &creature, const ItemEntity &item, const describe_option_type &opt);
