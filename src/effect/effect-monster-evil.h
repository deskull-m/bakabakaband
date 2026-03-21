#pragma once

#include "system/angband.h"

class EffectMonster;
class CreatureEntity;
ProcessResult effect_monster_away_undead(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_away_evil(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_away_all(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_turn_undead(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_turn_evil(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_turn_all(EffectMonster *em_ptr);
ProcessResult effect_monster_disp_undead(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_disp_evil(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_disp_good(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_disp_living(EffectMonster *em_ptr);
ProcessResult effect_monster_disp_demon(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_disp_all(EffectMonster *em_ptr);
