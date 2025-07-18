#pragma once

/*
 * @file player-status.h
 * @brief プレイヤーのステータスに関する状態取得処理ヘッダ
 */

#include <cstdint>
#include <string>

class ItemEntity;
class PlayerType;

int calc_weapon_weight_limit(PlayerType *player_ptr);
int calc_bow_weight_limit(PlayerType *player_ptr);
int calc_inventory_weight(PlayerType *player_ptr);

short calc_num_fire(PlayerType *player_ptr, const ItemEntity *o_ptr);
int calc_weight_limit(PlayerType *player_ptr);
void update_creature(PlayerType *player_ptr);
bool player_has_no_spellbooks(PlayerType *player_ptr);

void check_experience(PlayerType *player_ptr);
void wreck_the_pattern(PlayerType *player_ptr);
std::string cnv_stat(int val);
int16_t modify_stat_value(int value, int amount);
uint32_t calc_score(PlayerType *player_ptr);

bool is_blessed(PlayerType *player_ptr);
bool is_time_limit_esp(PlayerType *player_ptr);
bool is_time_limit_stealth(PlayerType *player_ptr);
bool is_fast(PlayerType *player_ptr);
bool is_invuln(PlayerType *player_ptr);
bool is_hero(PlayerType *player_ptr);
bool is_shero(PlayerType *player_ptr);
bool is_echizen(PlayerType *player_ptr);
bool is_tough(PlayerType *player_ptr);
bool is_chargeman(PlayerType *player_ptr);
bool is_sushi_eater(PlayerType *player_ptr);

void stop_mouth(PlayerType *player_ptr);

bool set_quick_and_tiny(PlayerType *player_ptr);
bool set_musasi(PlayerType *player_ptr);
bool set_icing_and_twinkle(PlayerType *player_ptr);
bool set_anubis_and_chariot(PlayerType *player_ptr);
