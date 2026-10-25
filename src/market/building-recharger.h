#pragma once

#include <cstdint>

class CreatureEntity;
class ItemEntity;
int64_t calc_recharge_all_price(const ItemEntity &item);
void building_recharge(CreatureEntity &creature);
void building_recharge_all(CreatureEntity &creature);
