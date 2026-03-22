#pragma once

#include <string_view>

enum class RandomArtActType : short;
class CreatureEntity;
class ItemEntity;
bool switch_activation(CreatureEntity &creature, ItemEntity **o_ptr_ptr, const RandomArtActType index, std::string_view name);
