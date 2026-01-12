#pragma once

#include "system/angband.h"

class CreatureEntity;
class PlayerType;
bool check_multishadow(const CreatureEntity &creature);
bool binding_field(PlayerType *player_ptr, int dam);
bool confusing_light(PlayerType *player_ptr);
bool set_multishadow(CreatureEntity &creature, TIME_EFFECT v, bool do_dec);
bool set_dustrobe(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);

enum class MindMirrorMasterType : int;
bool cast_mirror_spell(PlayerType *player_ptr, MindMirrorMasterType spell);
