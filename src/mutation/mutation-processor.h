#pragma once

class CreatureEntity;
void process_world_aux_sudden_attack(CreatureEntity &creature);
void process_world_aux_mutation(CreatureEntity &creature);
void process_monster_mutation(CreatureEntity &player, CreatureEntity &monster);
bool drop_weapons(CreatureEntity &creature);
