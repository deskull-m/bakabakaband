﻿#pragma once

#include "system/angband.h"

typedef struct object_type object_type;
class player_type;
void apply_magic_weapon(player_type *owner_ptr, object_type *o_ptr, DEPTH level, int power);
