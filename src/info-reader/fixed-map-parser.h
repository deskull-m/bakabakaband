#pragma once

#include <set>
#include <stdint.h>
#include <string_view>

class CreatureEntity;
enum parse_error_type : int;
enum class QuestId : short;
parse_error_type parse_fixed_map(CreatureEntity &creature, std::string_view name, int ymin, int xmin, int ymax, int xmax);
std::set<QuestId> parse_quest_info(std::string_view file_name);
