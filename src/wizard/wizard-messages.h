#pragma once

#include "system/angband.h"
#include <string_view>

class CreatureEntity;
void msg_print_wizard(CreatureEntity &creature, int cheat_type, std::string_view msg);
void msg_format_wizard(CreatureEntity &creature, int cheat_type, const char *fmt, ...);
