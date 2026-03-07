#pragma once

#include "system/angband.h"

class CreatureEntity;
class ItemTester;
bool get_item(CreatureEntity &creature, OBJECT_IDX *cp, concptr pmt, concptr str, BIT_FLAGS mode, const ItemTester &item_tester);
