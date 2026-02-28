#pragma once

class ItemEntity;
class CreatureEntity;
void reduce_lite_life(CreatureEntity &creature);
void notice_lite_change(CreatureEntity &creature, ItemEntity *o_ptr);
