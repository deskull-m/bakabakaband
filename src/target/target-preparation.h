#pragma once

#include "system/angband.h"
#include "util/point-2d.h"
#include <vector>

class CreatureEntity;
enum target_type : uint32_t;
bool target_able(CreatureEntity &creature, MONSTER_IDX m_idx);
std::vector<Pos2D> target_set_prepare(CreatureEntity &creature, target_type mode);
void target_sensing_monsters_prepare(CreatureEntity &creature, std::vector<MONSTER_IDX> &monster_list);
std::vector<MONSTER_IDX> target_pets_prepare(CreatureEntity &creature);
