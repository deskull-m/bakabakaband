#pragma once

#include <string>
#include <string_view>
#include <tl/optional.hpp>

class CreatureEntity;
tl::optional<std::string> resolve_common_expression_variable(CreatureEntity &creature, std::string_view name);
std::string process_pref_file_expr(CreatureEntity &creature, std::string_view expr);
