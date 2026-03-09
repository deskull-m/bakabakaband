#pragma once

#include "system/angband.h"

class CreatureEntity;
enum class RealmType;
MANA_POINT mod_need_mana(CreatureEntity &creature, MANA_POINT need_mana, SPELL_IDX spell_id, RealmType realm);
PERCENTAGE mod_spell_chance_1(CreatureEntity &creature, PERCENTAGE chance);
PERCENTAGE mod_spell_chance_2(CreatureEntity &creature, PERCENTAGE chance);
PERCENTAGE spell_chance(CreatureEntity &creature, SPELL_IDX spell_id, RealmType realm);
void print_spells(CreatureEntity &creature, SPELL_IDX target_spell_id, const SPELL_IDX *spell_ids, int num, TERM_LEN y, TERM_LEN x, RealmType realm);
