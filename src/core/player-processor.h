#pragma once

extern bool load; /*!<ロード処理中の分岐フラグ*/
extern bool can_save;

class CreatureEntity;
bool continuous_action_running(CreatureEntity &creature);
void process_player(CreatureEntity &creature);
void process_upkeep_with_speed(CreatureEntity &creature);
