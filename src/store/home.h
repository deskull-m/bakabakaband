#pragma once

class CreatureEntity;
class ItemEntity;
class Store;
enum class StoreSaleType;
int home_carry(CreatureEntity &creature, Store &store, ItemEntity *o_ptr);
bool combine_and_reorder_home(CreatureEntity &creature, Store &store);
bool combine_and_reorder_home(CreatureEntity &creature, const StoreSaleType store_num);
