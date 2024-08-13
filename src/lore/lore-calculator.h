#pragma once

#include "system/angband.h"
#include <string>

enum class MonsterAbilityType;
enum class MonsterRaceId : int16_t;

struct lore_type;
class PlayerType;
std::string dice_to_string(int base_damage, int dice_num, int dice_side, int dice_mult, int dice_div);
bool know_details(MonsterRaceId r_idx);
bool know_blow_damage(MonsterRaceId r_idx, int i);
void add_lore_of_damage_skill(PlayerType *player_ptr, lore_type *lore_ptr, MonsterAbilityType ms_type, concptr msg, byte color);
void set_flags_for_full_knowledge(lore_type *lore_ptr);
