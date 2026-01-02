#pragma once

class PlayerType;
class CreatureEntity;
void process_world_aux_sudden_attack(PlayerType *player_ptr);
void process_world_aux_mutation(PlayerType *player_ptr);
bool drop_weapons(CreatureEntity &creature);
