#pragma once
/*!
 * @file random-art-characteristics.h
 * @brief ランダムアーティファクトのバイアス付加処理ヘッダ
 */

#include <string>

class CreatureEntity;
class ItemEntity;
class PlayerType;
void curse_artifact(CreatureEntity &creature, ItemEntity *o_ptr);
std::string get_random_name(const ItemEntity &item, bool armour, int power);
bool has_extreme_damage_rate(CreatureEntity &creature, ItemEntity *o_ptr);
