#pragma once

class CreatureEntity;
struct self_info_type;
void display_life_rating(CreatureEntity &creature, self_info_type *self_ptr);
void display_max_base_status(CreatureEntity &creature, self_info_type *self_ptr);
void display_virtue(CreatureEntity &creature, self_info_type *self_ptr);
void display_mimic_race_ability(CreatureEntity &creature, self_info_type *self_ptr);
void display_self_info(self_info_type *self_ptr);
