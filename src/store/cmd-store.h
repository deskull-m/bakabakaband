#pragma once

#include <optional>

enum class StoreSaleType : int;
class PlayerType;
void do_cmd_store(PlayerType *player_ptr, std::optional<StoreSaleType> specified_store = std::nullopt);
