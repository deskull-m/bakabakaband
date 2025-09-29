#pragma once

#include <cstdint>
#include <string>
#include <tl/optional.hpp>

class PlayerType;
tl::optional<std::string> cave_gen(PlayerType *player_ptr, tl::optional<uint32_t> seed = tl::nullopt);
