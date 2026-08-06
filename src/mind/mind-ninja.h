#pragma once

struct player_attack_type;
class CreatureEntity;
bool kawarimi(CreatureEntity &creature, bool success);
bool rush_attack(CreatureEntity &creature, bool *mdeath);
void process_surprise_attack(CreatureEntity &creature, player_attack_type *pa_ptr);
void print_surprise_attack(player_attack_type *pa_ptr);
void calc_surprise_attack_damage(CreatureEntity &creature, player_attack_type *pa_ptr);
bool hayagake(CreatureEntity &creature);
bool set_superstealth(CreatureEntity &creature, bool set);

enum class MindNinjaType : int;
bool cast_ninja_spell(CreatureEntity &creature, MindNinjaType spell);
