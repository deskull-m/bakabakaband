#pragma once
#include "system/enums/dungeon/dungeon-id.h"
#include "system/h-type.h"

class CreatureEntity;

void leave_floor(CreatureEntity &creature);
void jump_floor(CreatureEntity &creature, DungeonId dun_idx, DEPTH depth);
