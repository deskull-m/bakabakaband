#pragma once

class CreatureEntity;
class MonsterEntity;
class PlayerType;
void print_tomb(CreatureEntity &creature);
void print_monster_tomb(PlayerType *player_ptr, MonsterEntity &monster);
void show_death_info(CreatureEntity &creature);
