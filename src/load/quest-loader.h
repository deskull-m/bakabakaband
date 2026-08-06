#pragma once

#include "system/angband.h"
#include <tuple>

class CreatureEntity;
errr load_town(void);
std::tuple<uint16_t, byte> load_quest_info();
void analyze_quests(CreatureEntity &creature, const uint16_t max_quests_load, const byte max_rquests_load);
