#pragma once

#include "system/angband.h"

enum class MonraceId : int16_t;
class FloorType;
class CreatureEntity;
class MonsterEntity;
bool monster_is_powerful(const FloorType &floor, MONSTER_IDX m_idx);
DEPTH monster_level_idx(const FloorType &floor, MONSTER_IDX m_idx);

int mon_damage_mod(CreatureEntity &creature, const MonsterEntity &monster, int dam, bool is_psy_spear);

void dispel_monster_status(CreatureEntity &creature, MONSTER_IDX m_idx);
void monster_gain_exp(CreatureEntity &creature, MONSTER_IDX m_idx, MonraceId s_idx);

enum class MonsterTimedEffect : int;
void process_monsters_mtimed(CreatureEntity &creature, MonsterTimedEffect mte);
