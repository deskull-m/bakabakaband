#pragma once

#include <optional>

enum class StoreSaleType : int;
class CreatureEntity;
void do_cmd_store(CreatureEntity &creature, std::optional<StoreSaleType> specified_store = std::nullopt);
