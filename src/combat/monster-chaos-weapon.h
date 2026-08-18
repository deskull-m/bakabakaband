#pragma once

#include "system/angband.h"

class CreatureEntity;
class ItemEntity;

bool apply_monster_weapon_chaos_effect(CreatureEntity &attacker, const ItemEntity &weapon, CreatureEntity &target,
    CreatureEntity &player, MONSTER_IDX attacker_m_idx, MONSTER_IDX target_m_idx, int weapon_damage);
