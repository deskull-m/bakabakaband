#pragma once
#include "system/enums/dungeon/dungeon-id.h"
#include "system/h-type.h"

class PlayerType;

void leave_floor(PlayerType *player_ptr);
void jump_floor(PlayerType *player_ptr, DungeonId dun_idx, DEPTH depth);
