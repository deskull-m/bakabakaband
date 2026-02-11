#pragma once

#include "system/angband.h"
#include <tl/optional.hpp>

enum summon_type : int;

class CreatureEntity;
class Direction;
class ItemEntity;
class PlayerType;
bool trump_summoning(CreatureEntity &creature, int num, bool pet, POSITION y, POSITION x, DEPTH lev, summon_type type, BIT_FLAGS mode);
bool cast_summon_demon(CreatureEntity &creature, int power);
bool cast_summon_undead(CreatureEntity &creature, int power);
bool cast_summon_nasty(CreatureEntity &creature, int power);
bool cast_summon_hound(CreatureEntity &creature, int power);
bool cast_summon_elemental(CreatureEntity &creature, int power);
bool cast_summon_octopus(CreatureEntity &creature);
bool cast_summon_greater_demon(CreatureEntity &creature);
bool summon_kin_player(CreatureEntity &creature, DEPTH level, POSITION y, POSITION x, BIT_FLAGS mode);
void mitokohmon(CreatureEntity &creature);
int summon_cyber(CreatureEntity &creature, POSITION y, POSITION x, tl::optional<MONSTER_IDX> summoner_m_idx = tl::nullopt);
int activate_hi_summon(CreatureEntity &creature, POSITION y, POSITION x, bool can_pet);
void cast_invoke_spirits(CreatureEntity &creature, const Direction &dir);
