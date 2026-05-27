/*
 * @file player-energy.cpp
 * @brief ゲームターン当たりの行動エネルギー増減処理
 * @author Hourier
 * @date 2021/04/29
 */

#include "player-status/player-energy.h"
#include "system/creature-entity.h"

PlayerEnergy::PlayerEnergy(CreatureEntity &creature)
{
    this->creature_ptr = &creature;
}

/*
 * @brief プレイヤーの行動エネルギーを更新する
 * @param need_cost 行動エネルギー
 * @param ut_type 現在値に対する演算方法
 */
void PlayerEnergy::set_player_turn_energy(ENERGY need_cost)
{
    this->creature_ptr->set_energy_use(need_cost);
}

void PlayerEnergy::add_player_turn_energy(ENERGY need_cost)
{
    this->creature_ptr->add_energy_use(need_cost);
}

void PlayerEnergy::sub_player_turn_energy(ENERGY need_cost)
{
    this->creature_ptr->sub_energy_use(need_cost);
}

void PlayerEnergy::mul_player_turn_energy(ENERGY need_cost)
{
    this->creature_ptr->mul_energy_use(need_cost);
}

void PlayerEnergy::div_player_turn_energy(ENERGY need_cost)
{
    this->creature_ptr->div_energy_use(need_cost);
}

void PlayerEnergy::reset_player_turn()
{
    set_player_turn_energy(0);
}
