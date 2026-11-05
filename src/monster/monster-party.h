#pragma once

#include "system/angband.h"

class CreatureEntity;
class FloorType;

void succeed_party_leader(FloorType &floor, MONSTER_IDX leaving_m_idx);
void leave_party_and_succeed_leader(FloorType &floor, CreatureEntity &monster);
void alert_party_members(CreatureEntity &creature, MONSTER_IDX waker_m_idx);
void collapse_party_morale(CreatureEntity &creature, MONSTER_IDX dead_m_idx);
