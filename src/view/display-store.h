#pragma once

class CreatureEntity;
class Store;
void store_prt_gold(int num_golds);
void display_entry(CreatureEntity &creature, const Store &store, int pos);
void display_store_inventory(CreatureEntity &creature, const Store &store);
void display_store(CreatureEntity &creature, const Store &store);
