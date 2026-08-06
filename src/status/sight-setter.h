#pragma once

#include "system/angband.h"

class PlayerType;
class CreatureEntity;
bool set_tim_esp(CreatureEntity &creature, TIME_EFFECT v, bool do_dec);
bool set_tim_invis(CreatureEntity &creature, TIME_EFFECT v, bool do_dec);
bool set_tim_infra(CreatureEntity &creature, TIME_EFFECT v, bool do_dec);
