#pragma once

#include "object/tval-types.h"
#include "system/angband.h"

class BaseitemKey;
class ItemEntity;
class PlayerType;
char index_to_label(int i);
int16_t wield_slot(PlayerType *player_ptr, const ItemEntity *o_ptr);
bool check_book_realm(PlayerType *player_ptr, const BaseitemKey &bi_key);
ItemEntity *ref_item(PlayerType *player_ptr, INVENTORY_IDX i_idx);
std::string activation_explanation(const ItemEntity *o_ptr);
