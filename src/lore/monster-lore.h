#pragma once

#include "lore/lore-util.h"
#include "system/angband.h"

class CreatureEntity;
void process_monster_lore(CreatureEntity &creature, MonraceId r_idx, monster_lore_mode mode);
