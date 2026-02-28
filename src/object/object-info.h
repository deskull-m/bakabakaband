#pragma once

#include "object/tval-types.h"
#include "system/angband.h"

class BaseitemKey;
class CreatureEntity;
class ItemEntity;
class CreatureEntity;
char index_to_label(int i);
int16_t wield_slot(CreatureEntity &creature, const ItemEntity *o_ptr);
bool check_book_realm(CreatureEntity &creature, const BaseitemKey &bi_key);
ItemEntity *ref_item(CreatureEntity &creature, INVENTORY_IDX i_idx);
std::string activation_explanation(const ItemEntity *o_ptr);
