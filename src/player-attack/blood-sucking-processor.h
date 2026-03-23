#pragma once

struct player_attack_type;
class CreatureEntity;
void decide_blood_sucking(CreatureEntity &creature, player_attack_type *pa_ptr);
void decide_exorcism(CreatureEntity &creature, player_attack_type *pa_ptr);
void calc_drain(player_attack_type *pa_ptr);
void process_drain(CreatureEntity &creature, player_attack_type *pa_ptr, const bool is_human, bool *drain_msg);
