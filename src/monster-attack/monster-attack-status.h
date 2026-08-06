#pragma once

#include "system/angband.h"

class CreatureEntity;
class MonsterAttackPlayer;
void process_blind_attack(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr);
void process_terrify_attack(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr);
void process_paralyze_attack(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr);
void process_lose_all_attack(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr);
void process_stun_attack(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr);
void process_groin_attack(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr);
void process_monster_attack_time(CreatureEntity &creature);
