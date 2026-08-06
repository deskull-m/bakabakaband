#pragma once

#include "system/angband.h"

class ItemEntity;
class CreatureEntity;
enum class CurseTraitType;
CurseTraitType get_curse(int power, ItemEntity *o_ptr);
void curse_equipment(CreatureEntity &creature, PERCENTAGE chance, PERCENTAGE heavy_chance);
