#pragma once

enum class StoreSaleType;
class CreatureEntity;
void store_prt_gold(int num_golds);
void display_entry(CreatureEntity &creature, int pos, StoreSaleType store_num);
void display_store_inventory(CreatureEntity &creature, StoreSaleType store_num);
void display_store(CreatureEntity &creature, StoreSaleType store_num);
