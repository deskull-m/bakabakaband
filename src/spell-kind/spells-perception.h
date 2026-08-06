#pragma once

#include "system/angband.h"

class ItemEntity;
class CreatureEntity;
void identify_pack(CreatureEntity &creature);
bool identify_item(CreatureEntity &creature, ItemEntity *o_ptr);
bool ident_spell(CreatureEntity &creature, bool only_equip);
bool identify_fully(CreatureEntity &creature, bool only_equip);
