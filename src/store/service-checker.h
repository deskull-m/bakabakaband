#pragma once

enum class StoreSaleType;
class CreatureEntity;
class ItemEntity;
bool store_will_buy(CreatureEntity &creature, const ItemEntity *o_ptr, StoreSaleType store_num);
void mass_produce(ItemEntity *o_ptr, StoreSaleType store_num);
