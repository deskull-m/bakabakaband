#pragma once

#include "system/angband.h"

#include "object-enchant/tr-flags.h"

class PlayerType;
class CreatureEntity;
void player_flags(PlayerType *player_ptr, TrFlags &flags);
void riding_flags(CreatureEntity &creature, TrFlags &flags, TrFlags &negative_flags);
