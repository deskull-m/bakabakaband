#pragma once

#include "system/angband.h"

class CreatureEntity;
class ItemEntity;
class PlayerType;
void generate_amusement(CreatureEntity &creature, int num, bool known);
void acquirement(CreatureEntity &creature, POSITION y1, POSITION x1, int num, bool great);
bool curse_armor(CreatureEntity &creature);
bool curse_weapon_object(CreatureEntity &creature, bool force, ItemEntity &item);
void brand_bolts(CreatureEntity &creature);

/*
 * Bit flags for the "enchant()" function
 */
#define ENCH_TOHIT 0x01 /*!< 装備強化処理: 命中強化 / Enchant to hit */
#define ENCH_TODAM 0x02 /*!< 装備強化処理: ダメージ強化 / Enchant to damage */
#define ENCH_TOAC 0x04 /*!< 装備強化処理: AC強化 / Enchant to AC */
#define ENCH_FORCE 0x08 /*!< 装備強化処理: 無条件に成功させる / Force enchantment */
bool enchant_equipment(ItemEntity &item, int n, int eflag);
bool enchant_spell(CreatureEntity &creature, HIT_PROB num_hit, int num_dam, ARMOUR_CLASS num_ac);
void brand_weapon(CreatureEntity &creature, int brand_type);
