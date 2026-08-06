#pragma once

#include "system/angband.h"

#include "combat/combat-options-type.h"
#include "object-enchant/tr-flags.h"

class CreatureEntity;
class MonsterAttackPlayer;
struct player_attack_type;
MULTIPLY mult_hissatsu(CreatureEntity &creature, MULTIPLY mult, const TrFlags &flags, const CreatureEntity &target, combat_options mode);
void concentration(CreatureEntity &creature);
bool choose_samurai_stance(CreatureEntity &creature);
int calc_attack_quality(CreatureEntity &creature, player_attack_type *pa_ptr);
void mineuchi(CreatureEntity &creature, player_attack_type *pa_ptr);
void musou_counterattack(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr);
