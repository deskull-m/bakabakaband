#pragma once

class MonsterAttackPlayer;
class CreatureEntity;
void calc_blow_disease(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr);
void calc_blow_lose_strength(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr);
void calc_blow_lose_intelligence(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr);
void calc_blow_lose_wisdom(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr);
void calc_blow_lose_dexterity(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr);
void calc_blow_lose_constitution(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr);
void calc_blow_lose_charisma(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr);
void calc_blow_lose_all(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr);
