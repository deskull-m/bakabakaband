#pragma once

#include "system/angband.h"

enum class MonsterAbilityType;
struct msa_type;
class CreatureEntity;
MonsterAbilityType choose_attack_spell(CreatureEntity &creature, msa_type *msa_ptr);
