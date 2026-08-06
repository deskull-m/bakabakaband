#pragma once

#include "system/angband.h"

#include "object-enchant/tr-flags.h"

class CreatureEntity;
void player_immunity(CreatureEntity &creature, TrFlags &flags);
void tim_player_immunity(CreatureEntity &creature, TrFlags &flags);
void known_obj_immunity(CreatureEntity &creature, TrFlags &flags);
void player_vulnerability_flags(CreatureEntity &creature, TrFlags &flags);
