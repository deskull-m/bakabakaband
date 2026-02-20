#pragma once

#include "system/angband.h"

class CreatureEntity;
int32_t get_current_ki(CreatureEntity &creature);
void set_current_ki(CreatureEntity &creature, bool is_reset, int32_t ki);
bool clear_mind(CreatureEntity &creature);
void set_lightspeed(CreatureEntity &creature, TIME_EFFECT v, bool do_dec);
bool set_tim_sh_force(CreatureEntity &creature, TIME_EFFECT v, bool do_dec);
bool shock_power(CreatureEntity &creature);

enum class MindForceTrainerType : int;
bool cast_force_spell(CreatureEntity &creature, MindForceTrainerType spell);
