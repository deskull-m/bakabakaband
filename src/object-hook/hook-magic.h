#pragma once

class ItemEntity;
class CreatureEntity;
bool object_is_activatable(const ItemEntity *o_ptr);
bool item_tester_hook_use(CreatureEntity &creature, const ItemEntity *o_ptr);
bool item_tester_learn_spell(CreatureEntity &creature, const ItemEntity *o_ptr);
