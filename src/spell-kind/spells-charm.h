#pragma once

#include "system/angband.h"

class CreatureEntity;
class Direction;
bool charm_monster(CreatureEntity &creature, const Direction &dir, PLAYER_LEVEL plev);
bool control_one_undead(CreatureEntity &creature, const Direction &dir, PLAYER_LEVEL plev);
bool control_one_demon(CreatureEntity &creature, const Direction &dir, PLAYER_LEVEL plev);
bool charm_animal(CreatureEntity &creature, const Direction &dir, PLAYER_LEVEL plev);
