#pragma once

#include <string>
#include <tl/optional.hpp>

extern tl::optional<std::string> histpref_buf;

class CreatureEntity;
int interpret_pref_file(CreatureEntity &creature, char *buf);
