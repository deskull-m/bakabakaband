#pragma once

#include <string>
#include <string_view>

class CreatureEntity;
void process_player_name(CreatureEntity &creature, bool is_new_savefile = false);
void get_name(CreatureEntity &creature);
std::string make_player_name_for_expression(std::string_view name);
