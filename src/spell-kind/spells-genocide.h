#pragma once

#include "system/angband.h"

class CreatureEntity;
bool genocide_aux(CreatureEntity &creature, MONSTER_IDX m_idx, int power, bool player_cast, int dam_side, concptr spell_name);
bool symbol_genocide(CreatureEntity &creature, int power, bool player_cast);
bool mass_genocide(CreatureEntity &creature, int power, bool player_cast);
bool mass_genocide_undead(CreatureEntity &creature, int power, bool player_cast);
