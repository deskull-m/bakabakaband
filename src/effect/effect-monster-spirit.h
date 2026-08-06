#pragma once

#include "system/angband.h"

class EffectMonster;
class CreatureEntity;
ProcessResult effect_monster_drain_mana(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_mind_blast(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_brain_smash(CreatureEntity &creature, EffectMonster *em_ptr);
