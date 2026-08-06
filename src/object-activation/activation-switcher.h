#pragma once

#include <memory>
#include <string_view>
#include <utility>

enum class RandomArtActType : short;
class CreatureEntity;
class ItemEntity;
class PlayerType;
std::pair<bool, std::shared_ptr<ItemEntity>> switch_activation(CreatureEntity &creature, ItemEntity &item, const RandomArtActType index, std::string_view name);
