#pragma once

#include "system/angband.h"

enum class MonraceId : int16_t;
class CreatureEntity;
class FloorType;
void set_pet(CreatureEntity &creature, CreatureEntity &target);
void anger_monster(CreatureEntity &creature, CreatureEntity &target);
bool set_monster_csleep(FloorType &floor, MONSTER_IDX m_idx, int v);
bool set_monster_fast(FloorType &floor, MONSTER_IDX m_idx, int v);
bool set_monster_slow(FloorType &floor, MONSTER_IDX m_idx, int v);
bool set_monster_stunned(FloorType &floor, MONSTER_IDX m_idx, int v);
bool set_monster_confused(FloorType &floor, MONSTER_IDX m_idx, int v);
bool set_monster_monfear(FloorType &floor, MONSTER_IDX m_idx, int v);
bool set_monster_invulner(FloorType &floor, MONSTER_IDX m_idx, int v, bool energy_need);
bool set_monster_timewalk(CreatureEntity &creature, MONSTER_IDX m_idx, int num, bool vs_player);
