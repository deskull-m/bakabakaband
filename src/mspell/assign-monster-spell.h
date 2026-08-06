#pragma once

#include "system/angband.h"

struct MonsterSpellResult;

enum class MonsterAbilityType;

class CreatureEntity;
MonsterSpellResult monspell_to_player(CreatureEntity &creature, MonsterAbilityType ms_type, POSITION y, POSITION x, MONSTER_IDX m_idx);
MonsterSpellResult monspell_to_monster(
    CreatureEntity &creature, MonsterAbilityType ms_type, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, bool is_special_spell);
