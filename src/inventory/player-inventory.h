#pragma once

#include "object/tval-types.h"
#include "system/angband.h"

class CreatureEntity;
class ItemTester;
bool can_get_item(CreatureEntity &creature, const ItemTester &item_tester);
void process_player_pickup_item(CreatureEntity &creature, OBJECT_IDX o_idx);
void carry(CreatureEntity &creature, bool pickup);
