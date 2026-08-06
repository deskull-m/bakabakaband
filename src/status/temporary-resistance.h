#pragma once

#include "system/angband.h"

class CreatureEntity;
bool set_tim_levitation(CreatureEntity &creature, TIME_EFFECT v, bool do_dec);
bool set_ultimate_res(CreatureEntity &creature, TIME_EFFECT v, bool do_dec);
bool set_tim_res_nether(CreatureEntity &creature, TIME_EFFECT v, bool do_dec);
bool set_tim_res_lite(CreatureEntity &creature, TIME_EFFECT v, bool do_dec);
bool set_tim_res_dark(CreatureEntity &creature, TIME_EFFECT v, bool do_dec);
bool set_tim_res_fear(CreatureEntity &creature, TIME_EFFECT v, bool do_dec);
bool set_tim_res_time(CreatureEntity &creature, TIME_EFFECT v, bool do_dec);
bool set_tim_imm_dark(CreatureEntity &creature, TIME_EFFECT v, bool do_dec);
