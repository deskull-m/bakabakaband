#pragma once

extern bool load; /*!<起動してから最初の process_player() までの間だけ真 (新規ゲームでは偽にする)。セーブの手番の途中から再開するかの判定に使う*/
extern bool can_save;

class CreatureEntity;
bool continuous_action_running(CreatureEntity &creature);
void process_player(CreatureEntity &creature);
void process_upkeep_with_speed(CreatureEntity &creature);
