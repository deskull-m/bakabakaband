#pragma once

#include "system/angband.h"

class EffectMonster;
class CreatureEntity;
ProcessResult effect_monster_lite_weak(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_lite(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_dark(CreatureEntity &creature, EffectMonster *em_ptr);
