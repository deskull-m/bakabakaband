#pragma once

#define LOW_PRICE_THRESHOLD 10L

class ItemEntity;
class CreatureEntity;
class Store;
int price_item(CreatureEntity &creature, const ItemEntity *o_ptr, const Store &store, bool flip);
