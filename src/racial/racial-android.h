#pragma once

#include <cstdint>

class CreatureEntity;
bool android_inside_weapon(CreatureEntity &creature);
void calc_android_exp(CreatureEntity &creature);
uint64_t android_item_experience(uint64_t value, int64_t level, bool special);
