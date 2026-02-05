#pragma once

#include "system/angband.h"
#include "util/point-2d.h"

class CreatureEntity;
MONSTER_NUMBER summon_EAGLE(CreatureEntity &creature, POSITION y, POSITION x, int rlev, MONSTER_IDX m_idx);
MONSTER_NUMBER summon_EDGE(CreatureEntity &creature, POSITION y, POSITION x, int rlev, MONSTER_IDX m_idx);
MONSTER_NUMBER summon_guardian(CreatureEntity &creature, POSITION y, POSITION x, int rlev, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MONSTER_NUMBER summon_LOCKE_CLONE(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx);
MONSTER_NUMBER summon_LOUSE(CreatureEntity &creature, POSITION y, POSITION x, int rlev, MONSTER_IDX m_idx);
MONSTER_NUMBER summon_MOAI(CreatureEntity &creature, POSITION y, POSITION x, int rlev, MONSTER_IDX m_idx);
MONSTER_NUMBER summon_DEMON_SLAYER(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx);
MONSTER_NUMBER summon_NAZGUL(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx);
MONSTER_NUMBER summon_APOCRYPHA(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx);
MONSTER_NUMBER summon_HIGHEST_DRAGON(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx);
MONSTER_NUMBER summon_PYRAMID(CreatureEntity &creature, POSITION y, POSITION x, int rlev, MONSTER_IDX m_idx);
MONSTER_NUMBER summon_EYE_PHORN(CreatureEntity &creature, POSITION y, POSITION x, int rlev, MONSTER_IDX m_idx);
MONSTER_NUMBER summon_VESPOID(CreatureEntity &creature, POSITION y, POSITION x, int rlev, MONSTER_IDX m_idx);
MONSTER_NUMBER summon_YENDER_WIZARD(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx);
MONSTER_NUMBER summon_THUNDERS(CreatureEntity &creature, POSITION y, POSITION x, int rlev, MONSTER_IDX m_idx);
MONSTER_NUMBER summon_PLASMA(CreatureEntity &creature, POSITION y, POSITION x, int rlev, MONSTER_IDX m_idx);
MONSTER_NUMBER summon_LAFFEY_II(CreatureEntity &creature, const Pos2D &position, MONSTER_IDX m_idx);
MONSTER_NUMBER summon_POLYGON(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx);
