#pragma once

#include "dungeon/quest.h"
#include "system/angband.h"

class PlayerType;
class QuestType;
void load_zangband_options();
void set_zangband_realm(PlayerType *player_ptr);
void set_zangband_skill(PlayerType *player_ptr);
void set_zangband_race(PlayerType *player_ptr);
void set_zangband_bounty_uniques(PlayerType *player_ptr);
void set_zangband_mimic(PlayerType *player_ptr);
void set_zangband_holy_aura(PlayerType *player_ptr);
void set_zangband_reflection(PlayerType *player_ptr);
void rd_zangband_dungeon();
void set_zangband_game_turns(PlayerType *player_ptr);
void set_zangband_special_attack(PlayerType *player_ptr);
void set_zangband_special_defense(PlayerType *player_ptr);
void set_zangband_action(PlayerType *player_ptr);
void set_zangband_learnt_spells(PlayerType *player_ptr);
void set_zangband_pet(PlayerType *player_ptr);
