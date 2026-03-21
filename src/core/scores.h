#pragma once

#include "system/angband.h"

class CreatureEntity;
bool send_world_score(CreatureEntity &creature, bool do_send);
errr top_twenty(CreatureEntity &creature);
errr predict_score(CreatureEntity &creature);
void race_legends(CreatureEntity &creature);
void race_score(CreatureEntity &creature, int race_num);
void show_highclass(CreatureEntity &creature);
bool check_score(CreatureEntity &creature);
