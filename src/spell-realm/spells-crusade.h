#pragma once

#include "system/angband.h"

class CreatureEntity;
class PlayerType;
bool set_tim_sh_holy(CreatureEntity &creature, TIME_EFFECT v, bool do_dec);
bool set_tim_eyeeye(CreatureEntity &creature, TIME_EFFECT v, bool do_dec);
void check_emission(CreatureEntity &creature);
void check_demigod(CreatureEntity &creature);
bool has_slay_demon_from_exorcism(const CreatureEntity &creature);
bool has_kill_demon_from_exorcism(const CreatureEntity &creature);
bool has_slay_undead_from_exorcism(const CreatureEntity &creature);
bool has_kill_undead_from_exorcism(const CreatureEntity &creature);
const auto THRESHOLD_KILL_FROM_EXORCISM = 45;
