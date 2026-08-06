#pragma once

#include "combat/combat-options-type.h"
#include "system/angband.h"
#include "util/point-2d.h"

class CreatureEntity;
bool do_cmd_attack(CreatureEntity &creature, POSITION y, POSITION x, combat_options mode);
void attack_monster_in_the_way(CreatureEntity &creature, const Pos2D &pos);
bool do_cmd_headbutt(CreatureEntity &creature);
void do_cmd_body_slam(CreatureEntity &creature);
void do_cmd_enema(CreatureEntity &creature);
