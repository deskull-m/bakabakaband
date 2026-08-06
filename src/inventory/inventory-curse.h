#pragma once

#include "object-enchant/trc-types.h"
#include "system/angband.h"

class CreatureEntity;
class ItemEntity;
class PlayerType;
ItemEntity *choose_cursed_obj_name(CreatureEntity &creature, CurseTraitType flag);
void execute_cursed_items_effect(CreatureEntity &creature);
