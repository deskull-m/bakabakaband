﻿#pragma once

#include "object/object-broken.h"

/*
 * This seems like a pretty standard "typedef"
 */
typedef struct object_type object_type;
class player_type;

void inventory_damage(player_type *creature_ptr, const ObjectBreaker& breaker, int perc);
