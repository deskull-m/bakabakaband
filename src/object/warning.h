#pragma once

#include "system/angband.h"

class ItemEntity;
class CreatureEntity;
ItemEntity *choose_warning_item(CreatureEntity &creature);
bool process_warning(CreatureEntity &creature, POSITION xx, POSITION yy);
