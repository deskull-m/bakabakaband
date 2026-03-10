#pragma once

class CreatureEntity;
class ItemEntity;
enum class StoreSaleType;
int home_carry(CreatureEntity &creature, ItemEntity *o_ptr, StoreSaleType store_num);
bool combine_and_reorder_home(CreatureEntity &creature, const StoreSaleType store_num);
