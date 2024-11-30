#pragma once
#include "system/h-type.h"

class PlayerType;
void leave_floor(PlayerType *player_ptr);
void jump_floor(PlayerType *player_ptr, DUNGEON_IDX dun_idx, DEPTH depth);
