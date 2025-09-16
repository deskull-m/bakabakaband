#pragma once

#include "combat/combat-options-type.h"
#include "system/angband.h"

class PlayerType;
bool do_cmd_attack(PlayerType *player_ptr, POSITION y, POSITION x, combat_options mode);
bool do_cmd_headbutt(PlayerType *player_ptr);
void do_cmd_body_slam(PlayerType *player_ptr);
