#pragma once

#include "system/angband.h"

#define MPE_STAYING 0x00000001
#define MPE_FORGET_FLOW 0x00000002
#define MPE_HANDLE_STUFF 0x00000004
#define MPE_ENERGY_USE 0x00000008
#define MPE_DONT_PICKUP 0x00000010
#define MPE_DO_PICKUP 0x00000020
#define MPE_BREAK_TRAP 0x00000040
#define MPE_DONT_SWAP_MON 0x00000080

class CreatureEntity;
bool move_player_effect(CreatureEntity &creature, POSITION ny, POSITION nx, BIT_FLAGS mpe_mode);
bool trap_can_be_ignored(CreatureEntity &creature, FEAT_IDX feat);
void search(CreatureEntity &creature);
