#pragma once

#include "object/tval-types.h"

class CreatureEntity;
class PlayerType;
class ItemTester;
void display_inventory(CreatureEntity &creature, const ItemTester &item_tester);
