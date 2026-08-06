#pragma once

#include "system/angband.h"

class CreatureEntity;
class ItemTester;
bool enchant_item(CreatureEntity &creature, PRICE cost, HIT_PROB to_hit, int to_dam, ARMOUR_CLASS to_ac, const ItemTester &item_tester);
