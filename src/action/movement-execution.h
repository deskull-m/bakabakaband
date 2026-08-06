#pragma once
/*!
 * @file movement-execution.h
 * @brief プレイヤーの歩行処理実行ヘッダ
 */

class Direction;
class CreatureEntity;
void exe_movement(CreatureEntity &creature, const Direction &dir, bool do_pickup, bool break_trap);
