#pragma once

#include "system/angband.h"
#include <tl/optional.hpp>

class EffectMonster;
class CapturedMonsterType;
class CreatureEntity;
ProcessResult switch_effects_monster(CreatureEntity &creature, EffectMonster *em_ptr, tl::optional<CapturedMonsterType *> cap_mon_ptr = tl::nullopt);
