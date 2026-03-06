#pragma once

#include "object/object-broken.h"

/*
 * This seems like a pretty standard "typedef"
 */
class ItemEntity;
class CreatureEntity;

void inventory_damage(CreatureEntity &creature, const ObjectBreaker &breaker, int perc);
