#pragma once

/*!
 * @file magic-device-shatter.h
 * @brief 魔道具 (杖/魔法棒/ロッド) を自ら砕いたときの魔力暴発 (Issue #1770)
 */

class CreatureEntity;
class ItemEntity;

int count_device_remaining_charges(const ItemEntity &item, int amount);
int calc_device_shatter_damage(int charges, int item_level);
int calc_device_shatter_self_damage(int ball_damage);
int calc_device_shatter_radius(int item_level);
bool shatter_magic_device(CreatureEntity &creature, const ItemEntity &item, int amount);
