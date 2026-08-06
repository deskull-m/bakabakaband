#pragma once

#include "player-info/race-types.h"
#include "system/angband.h"

class CreatureEntity;
void do_poly_self(CreatureEntity &creature);
void do_poly_wounds(CreatureEntity &creature);
void change_race(CreatureEntity &creature, PlayerRaceType new_race, concptr effect_msg);
