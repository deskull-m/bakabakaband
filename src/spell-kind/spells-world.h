#pragma once

#include "system/angband.h"

class CreatureEntity;
void teleport_level(CreatureEntity &creature, MONSTER_IDX m_idx);
bool teleport_level_other(CreatureEntity &creature);
bool tele_town(CreatureEntity &creature);
void reserve_alter_reality(CreatureEntity &creature, TIME_EFFECT turns);
bool recall_player(CreatureEntity &creature, TIME_EFFECT turns);
bool free_level_recall(CreatureEntity &creature);
bool reset_recall(CreatureEntity &creature);
