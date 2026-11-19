#pragma once

class CreatureEntity;
class StoreScreen;
void store_prt_gold(const StoreScreen &screen, int num_golds);
void display_entry(CreatureEntity &creature, const StoreScreen &screen, int pos);
void display_store_inventory(CreatureEntity &creature, const StoreScreen &screen);
void display_store(CreatureEntity &creature, const StoreScreen &screen);
