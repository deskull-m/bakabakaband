#pragma once

class ItemEntity;
class CreatureEntity;
bool item_tester_hook_eatable(CreatureEntity &creature, const ItemEntity *o_ptr);
bool item_tester_hook_quaff(CreatureEntity &creature, const ItemEntity *o_ptr);
bool can_player_destroy_object(ItemEntity *o_ptr);
