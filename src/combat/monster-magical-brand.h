#pragma once

#include "system/angband.h"

enum class MagicalBrandEffectType;
class CreatureEntity;
class ItemEntity;

MagicalBrandEffectType roll_monster_magical_brand_effect(const ItemEntity &weapon);
int monster_magical_brand_extra_dice(MagicalBrandEffectType effect);
void apply_monster_magical_brand_status(CreatureEntity &attacker, MagicalBrandEffectType effect, CreatureEntity &target,
    CreatureEntity &player, MONSTER_IDX target_m_idx);
