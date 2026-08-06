#pragma once

#include "system/angband.h"

class CreatureEntity;
bool check_multishadow(const CreatureEntity &creature);
bool binding_field(CreatureEntity &creature, int dam);
bool confusing_light(CreatureEntity &creature);
bool set_multishadow(CreatureEntity &creature, TIME_EFFECT v, bool do_dec);
bool set_dustrobe(CreatureEntity &creature, TIME_EFFECT v, bool do_dec);

enum class MindMirrorMasterType : int;
bool cast_mirror_spell(CreatureEntity &creature, MindMirrorMasterType spell);
