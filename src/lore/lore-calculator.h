#pragma once

#include "system/angband.h"
#include <string>

enum class MonsterAbilityType;
enum class MonraceId : int16_t;

struct lore_type;
class CreatureEntity;
std::string dice_to_string(int base_damage, int dice_num, int dice_side, int dice_mult, int dice_div);
std::string get_skill_damage(CreatureEntity &creature, lore_type *lore_ptr, MonsterAbilityType ms_type);
void add_lore_of_damage_skill(CreatureEntity &creature, lore_type *lore_ptr, MonsterAbilityType ms_type, concptr msg, byte color);
void set_flags_for_full_knowledge(lore_type *lore_ptr);
