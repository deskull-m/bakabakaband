/*!
 * @file monster-corpse-leaver.h
 * @brief モンスターの死体をフロアへ遺す処理
 */

#pragma once

#include "util/point-2d.h"
#include <vector>

class CreatureEntity;
class FloorType;
class MonsterCorpse;

short leave_monster_corpse(CreatureEntity &creature, short m_idx);
std::vector<short> collect_corpses_at(const FloorType &floor, const Pos2D &pos);
void wipe_corpses(FloorType &floor);
