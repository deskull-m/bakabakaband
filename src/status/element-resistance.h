#pragma once

#include "system/angband.h"

class CreatureEntity;
bool set_oppose_acid(CreatureEntity &creature, TIME_EFFECT v, bool do_dec);
bool set_oppose_elec(CreatureEntity &creature, TIME_EFFECT v, bool do_dec);
bool set_oppose_fire(CreatureEntity &creature, TIME_EFFECT v, bool do_dec);
bool set_oppose_cold(CreatureEntity &creature, TIME_EFFECT v, bool do_dec);
bool set_oppose_pois(CreatureEntity &creature, TIME_EFFECT v, bool do_dec);
bool is_oppose_acid(CreatureEntity &creature);
bool is_oppose_elec(CreatureEntity &creature);
bool is_oppose_fire(CreatureEntity &creature);
bool is_oppose_cold(CreatureEntity &creature);
bool is_oppose_pois(CreatureEntity &creature);
