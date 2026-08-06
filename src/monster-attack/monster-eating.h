#pragma once

#include "system/angband.h"

class CreatureEntity;
class MonsterAttackPlayer;
void process_eat_gold(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr);
bool check_eat_item(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr);
void process_eat_item(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr);
void process_eat_food(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr);
void process_eat_lite(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr);

bool process_un_power(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr);
bool check_drain_hp(CreatureEntity &creature, const int32_t d);
void process_drain_life(MonsterAttackPlayer *monap_ptr, const bool resist_drain);
void process_drain_mana(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr);
void process_monster_attack_hungry(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr);
