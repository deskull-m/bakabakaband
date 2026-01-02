#pragma once

#include "system/angband.h"

class PlayerType;
class CreatureEntity;
int adjust_stat(int value, int amount);
void get_stats(CreatureEntity *creature_ptr);
void get_money_for_creature(CreatureEntity *creature_ptr);
uint16_t get_expfact(PlayerType *player_ptr);
void get_extra(PlayerType *player_ptr, bool roll_hitdie);

void get_max_stats(PlayerType *player_ptr);
