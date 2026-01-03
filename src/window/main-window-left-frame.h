#pragma once

class PlayerType;
class CreatureEntity;
void print_title(PlayerType *player_ptr);
void print_level(PlayerType *player_ptr);
void print_exp(PlayerType *player_ptr);
void print_ac(PlayerType *player_ptr);
void print_hp(CreatureEntity &creature);
void print_sp(CreatureEntity &creature);
void print_gold(CreatureEntity &creature);
void print_depth(PlayerType *player_ptr);
void print_frame_basic(PlayerType *player_ptr);
void print_health(PlayerType *player_ptr, bool riding);
