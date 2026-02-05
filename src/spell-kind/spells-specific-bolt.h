#pragma once

#include "system/angband.h"

class CreatureEntity;
class Direction;
bool hypodynamic_bolt(CreatureEntity &creature, const Direction &dir, int dam);
bool death_ray(CreatureEntity &creature, const Direction &dir, PLAYER_LEVEL plev);
