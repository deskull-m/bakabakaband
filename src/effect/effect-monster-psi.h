#pragma once

#include "system/angband.h"

class EffectMonster;
class CreatureEntity;
ProcessResult effect_monster_psi(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_psi_drain(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_telekinesis(EffectMonster *em_ptr);
