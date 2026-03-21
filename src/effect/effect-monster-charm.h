#pragma once

#include "system/angband.h"
#include <tl/optional.hpp>

enum class ProcessResult;
class EffectMonster;
class CapturedMonsterType;
class CreatureEntity;
ProcessResult effect_monster_charm(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_control_undead(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_control_demon(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_control_animal(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_charm_living(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_domination(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_crusade(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_capture(CreatureEntity &creature, EffectMonster *em_ptr, tl::optional<CapturedMonsterType *> cap_mon_ptr);
