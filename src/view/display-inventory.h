#pragma once

#include "system/angband.h"

class CreatureEntity;
class PlayerType;
class ItemTester;
COMMAND_CODE show_inventory(CreatureEntity &creature, int target_item, BIT_FLAGS mode, const ItemTester &item_tester);
COMMAND_CODE show_equipment(CreatureEntity &creature, int target_item, BIT_FLAGS mode, const ItemTester &item_tester);
