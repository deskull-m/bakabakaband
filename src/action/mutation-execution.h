#pragma once
/*!
 * @file mutation-execution.h
 * @brief プレイヤーの変異能力実行ヘッダ
 */

enum class PlayerMutationType;
class CreatureEntity;
bool exe_mutation_power(CreatureEntity &creature, PlayerMutationType power);
