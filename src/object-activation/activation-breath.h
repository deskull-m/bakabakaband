#pragma once

class CreatureEntity;
class ItemEntity;
bool activate_dragon_breath(CreatureEntity &creature, ItemEntity *o_ptr);
bool activate_breath_fire(CreatureEntity &creature, ItemEntity *o_ptr);
bool activate_breath_cold(CreatureEntity &creature, ItemEntity *o_ptr);
