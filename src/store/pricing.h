#pragma once

#define LOW_PRICE_THRESHOLD 10L

enum class StoreSaleType;
class ItemEntity;
class CreatureEntity;
int price_item(CreatureEntity &creature, const ItemEntity *o_ptr, int greed, bool flip, StoreSaleType store_num);
