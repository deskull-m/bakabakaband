#pragma once

#include "system/angband.h"

class CreatureEntity;
void reset_concentration(CreatureEntity &creature, bool msg);
void display_snipe_list(CreatureEntity &creature);
MULTIPLY calc_snipe_damage_with_slay(CreatureEntity &creature, MULTIPLY mult, const CreatureEntity &target, SPELL_IDX snipe_type);
void do_cmd_snipe(CreatureEntity &creature);
void do_cmd_snipe_browse(CreatureEntity &creature);
int boost_concentration_damage(CreatureEntity &creature, int tdam);
