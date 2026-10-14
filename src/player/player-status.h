#pragma once

/*
 * @file player-status.h
 * @brief プレイヤーのステータスに関する状態取得処理ヘッダ
 */

#include <cstdint>
#include <string>

class ItemEntity;
class PlayerType;
class CreatureEntity;

int calc_weapon_weight_limit(CreatureEntity &creature);
int calc_bow_weight_limit(CreatureEntity &creature);
int calc_inventory_weight(CreatureEntity &creature);

short calc_num_fire(CreatureEntity &creature, const ItemEntity *o_ptr);
int calc_weight_limit(CreatureEntity &creature);
int calc_creature_mana(CreatureEntity &creature);
void update_creature(CreatureEntity &creature);
bool player_has_no_spellbooks(CreatureEntity &creature);

void check_experience(CreatureEntity &creature);
void wreck_the_pattern(CreatureEntity &creature);
std::string cnv_stat(int val);
int16_t modify_stat_value(int value, int amount);
uint32_t calc_score(CreatureEntity &creature);

void stop_mouth(CreatureEntity &creature);

bool set_quick_and_tiny(CreatureEntity &creature);
bool set_musasi(CreatureEntity &creature);
bool set_icing_and_twinkle(CreatureEntity &creature);
bool set_anubis_and_chariot(CreatureEntity &creature);
