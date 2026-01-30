#pragma once

#include "system/angband.h"

class CreatureEntity;
class PlayerType;
bool set_tim_sh_holy(CreatureEntity &subject, TIME_EFFECT v, bool do_dec);
bool set_tim_eyeeye(CreatureEntity &subject, TIME_EFFECT v, bool do_dec);
void check_emission(CreatureEntity &subject);
void check_demigod(CreatureEntity &subject);
bool has_slay_demon_from_exorcism(const CreatureEntity &subject);
bool has_kill_demon_from_exorcism(const CreatureEntity &subject);
bool has_slay_undead_from_exorcism(const CreatureEntity &subject);
bool has_kill_undead_from_exorcism(const CreatureEntity &subject);
const auto THRESHOLD_KILL_FROM_EXORCISM = 45;
