#pragma once

struct player_attack_type;
class CreatureEntity;
void process_vorpal_attack(CreatureEntity &creature, player_attack_type *pa_ptr, const bool vorpal_cut, const int vorpal_chance);
