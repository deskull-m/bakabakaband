#pragma once

#include <string_view>

enum class GameOptionPage : int;
class CreatureEntity;
void extract_option_vars();
void do_cmd_options_aux(CreatureEntity &creature, GameOptionPage page, std::string_view info);
void do_cmd_options(CreatureEntity &creature);
