#pragma once

#include "system/angband.h"

class CreatureEntity;
class ItemEntity;
bool object_sort_comp(CreatureEntity &creature, const ItemEntity &item1, const ItemEntity &item2);
