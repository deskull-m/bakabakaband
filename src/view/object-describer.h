#pragma once

class ItemEntity;
class CreatureEntity;
void inven_item_charges(const ItemEntity &item);
void inven_item_describe(CreatureEntity &creature, short i_idx);
void display_koff(CreatureEntity &creature);
