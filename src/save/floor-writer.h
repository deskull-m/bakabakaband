#pragma once

#include "system/angband.h"

/* Flags for save/load temporary saved floor file */
#define SLF_SECOND 0x0001 /* Called from another save/load function */
#define SLF_NO_KILL 0x0002 /* Don't kill temporary files */

class CreatureEntity;
struct saved_floor_type;
void wr_saved_floor(CreatureEntity &creature, saved_floor_type *sf_ptr);
bool wr_dungeon(CreatureEntity &creature);
bool save_floor(CreatureEntity &creature, saved_floor_type *sf_ptr, BIT_FLAGS mode);
