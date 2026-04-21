#pragma once

#include "system/angband.h"

class CreatureEntity;
int adjust_stat(int value, int amount);
void get_stats(CreatureEntity &creature);
void get_money_for_creature(CreatureEntity &creature);
uint16_t get_expfact(CreatureEntity &creature);
void get_extra(CreatureEntity &creature, bool roll_hitdie);

void get_max_stats(CreatureEntity &creature);
