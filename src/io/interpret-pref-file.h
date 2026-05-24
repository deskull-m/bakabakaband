#pragma once

#include <string>
#include <string_view>
#include <tl/optional.hpp>

extern tl::optional<std::string> histpref_buf;

class CreatureEntity;
int interpret_pref_file(CreatureEntity &creature, std::string_view buf);
