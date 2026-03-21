#pragma once
/*!
 * @file action-limited.h
 * @brief プレイヤーの行動制約判定ヘッダ
 */

class CreatureEntity;
bool cmd_limit_cast(CreatureEntity &creature);
bool cmd_limit_arena(const CreatureEntity &creature);
bool cmd_limit_time_walk(CreatureEntity &creature);
bool cmd_limit_blind(CreatureEntity &creature);
bool cmd_limit_confused(const CreatureEntity &creature);
bool cmd_limit_image(const CreatureEntity &creature);
bool cmd_limit_stun(const CreatureEntity &creature);
