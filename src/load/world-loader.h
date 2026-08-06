#pragma once

#include "system/angband.h"

class CreatureEntity;
void rd_dungeons(CreatureEntity &creature);
void set_gambling_monsters(void);
void rd_autopick(CreatureEntity &creature);
void rd_global_configurations(CreatureEntity &creature);
void load_wilderness_info(CreatureEntity &creature);
errr analyze_wilderness(void);
