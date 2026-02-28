#pragma once

#include "system/angband.h"
#include <string>

class CreatureEntity;
std::string process_pref_file_expr(CreatureEntity &creature, char **sp, char *fp);
