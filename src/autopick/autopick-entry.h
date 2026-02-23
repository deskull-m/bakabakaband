#pragma once

#include <string>
#include <string_view>

struct autopick_type;
class CreatureEntity;
class ItemEntity;
class PlayerType;
bool autopick_new_entry(autopick_type *entry, std::string_view str, bool allow_default);
void autopick_entry_from_object(CreatureEntity &creature, autopick_type *entry, const ItemEntity *o_ptr);
std::string autopick_line_from_entry(const autopick_type &entry);
bool entry_from_choosed_object(CreatureEntity &creature, autopick_type *entry);
