#pragma once

#include "system/angband.h"

struct MonsterSpellResult;

class CreatureEntity;
MonsterSpellResult spell_RF4_SHRIEK(MONSTER_IDX m_idx, CreatureEntity &creature, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_WORLD(CreatureEntity &creature, MONSTER_IDX m_idx);
MonsterSpellResult spell_RF6_BLINK(CreatureEntity &creature, MONSTER_IDX m_idx, int target_type, bool is_quantum_effect);
MonsterSpellResult spell_RF6_TPORT(CreatureEntity &creature, MONSTER_IDX m_idx, int target_type);
MonsterSpellResult spell_RF6_TELE_TO(CreatureEntity &creature, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_TELE_AWAY(CreatureEntity &creature, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_TELE_LEVEL(CreatureEntity &creature, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_DARKNESS(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_TRAPS(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx);
MonsterSpellResult spell_RF6_RAISE_DEAD(CreatureEntity &creature, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
