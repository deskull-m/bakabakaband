#pragma once

#include "effect/attribute-types.h"
#include "system/angband.h"
#include <string>

class CreatureEntity;
class MonraceDefinition;
class PlayerType;
bool project_all_los(CreatureEntity &creature, AttributeType typ, int dam);
bool speed_monsters(CreatureEntity &creature);
bool slow_monsters(CreatureEntity &creature, int power);
bool sleep_monsters(CreatureEntity &creature, int power);
void aggravate_monsters(CreatureEntity &creature, MONSTER_IDX src_idx);
bool banish_evil(CreatureEntity &creature, int dist);
bool turn_undead(CreatureEntity &creature);
bool dispel_evil(CreatureEntity &creature, int dam);
bool dispel_good(CreatureEntity &creature, int dam);
bool dispel_undead(CreatureEntity &creature, int dam);
bool dispel_monsters(CreatureEntity &creature, int dam);
bool dispel_living(CreatureEntity &creature, int dam);
bool dispel_demons(CreatureEntity &creature, int dam);
bool crusade(CreatureEntity &creature);
bool confuse_monsters(CreatureEntity &creature, int dam);
bool charm_monsters(CreatureEntity &creature, int dam);
bool charm_animals(CreatureEntity &creature, int dam);
bool stun_monsters(CreatureEntity &creature, int dam);
bool stasis_monsters(CreatureEntity &creature, int dam);
bool mindblast_monsters(CreatureEntity &creature, int dam);
bool banish_monsters(CreatureEntity &creature, int dist);
bool turn_evil(CreatureEntity &creature, int dam);
bool turn_monsters(CreatureEntity &creature, int dam);
bool deathray_monsters(CreatureEntity &creature);
std::string probed_monster_info(CreatureEntity &creature, CreatureEntity &target, const MonraceDefinition &monrace);
bool probing(CreatureEntity &creature);
