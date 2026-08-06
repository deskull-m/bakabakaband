#pragma once

#include "system/angband.h"

enum summon_type : int;
enum class MonraceId : int16_t;
class CreatureEntity;
bool check_summon_specific(CreatureEntity &creature, MonraceId summoner_idx, MonraceId r_idx, summon_type type);
