#pragma once

#include "system/angband.h"

enum class MimicKindType;
class CreatureEntity;
void reset_tim_flags(CreatureEntity &creature);
bool set_acceleration(CreatureEntity &creature, TIME_EFFECT v, bool do_dec);
bool mod_acceleration(CreatureEntity &creature, const TIME_EFFECT v, const bool do_dec);
bool set_shield(CreatureEntity &creature, TIME_EFFECT v, bool do_dec);
bool set_magicdef(CreatureEntity &creature, TIME_EFFECT v, bool do_dec);
bool set_blessed(CreatureEntity &creature, TIME_EFFECT v, bool do_dec);
bool set_hero(CreatureEntity &creature, TIME_EFFECT v, bool do_dec);
bool set_mimic(CreatureEntity &creature, TIME_EFFECT v, MimicKindType mimic_race_idx, bool do_dec);
bool set_berserk(CreatureEntity &creature, TIME_EFFECT v, bool do_dec);
bool set_wraith_form(CreatureEntity &creature, TIME_EFFECT v, bool do_dec);
bool set_tsuyoshi(CreatureEntity &creature, TIME_EFFECT v, bool do_dec);
