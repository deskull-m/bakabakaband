#pragma once

#include "system/angband.h"

class CreatureEntity;
class PlayerType;
void call_the_void(CreatureEntity &creature);
bool vanish_dungeon(CreatureEntity &creature);
void cast_meteor(CreatureEntity &creature, int dam, POSITION rad);
