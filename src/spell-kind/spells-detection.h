#pragma once

#include "system/angband.h"

class CreatureEntity;
bool detect_traps(CreatureEntity &creature, POSITION range, bool known);
bool detect_doors(CreatureEntity &creature, POSITION range);
bool detect_stairs(CreatureEntity &creature, POSITION range);
bool detect_treasure(CreatureEntity &creature, POSITION range);
bool detect_objects_gold(CreatureEntity &creature, POSITION range);
bool detect_objects_normal(CreatureEntity &creature, POSITION range);
bool detect_objects_magic(CreatureEntity &creature, POSITION range);
bool detect_monsters_normal(CreatureEntity &creature, POSITION range);
bool detect_monsters_invis(CreatureEntity &creature, POSITION range);
bool detect_monsters_evil(CreatureEntity &creature, POSITION range);
bool detect_monsters_xxx(CreatureEntity &creature, POSITION range, uint32_t match_flag);
bool detect_monsters_string(CreatureEntity &creature, POSITION range, concptr);
bool detect_monsters_nonliving(CreatureEntity &creature, POSITION range);
bool detect_monsters_mind(CreatureEntity &creature, POSITION range);
bool detect_all(CreatureEntity &creature, POSITION range);
