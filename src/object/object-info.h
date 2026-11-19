#pragma once

#include <memory>
#include <span>
#include <string>
#include <tl/optional.hpp>

class BaseitemKey;
class CreatureEntity;
class ItemEntity;
char index_to_label(int i);
short wield_slot(CreatureEntity &creature, const ItemEntity &item);
bool check_book_realm(CreatureEntity &creature, const BaseitemKey &bi_key);
std::shared_ptr<ItemEntity> ref_item(CreatureEntity &creature, short i_idx);
tl::optional<short> find_item_index(std::span<const std::shared_ptr<ItemEntity>> items, const std::shared_ptr<ItemEntity> &item);
std::string activation_explanation(const ItemEntity *o_ptr);
