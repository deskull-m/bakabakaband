#pragma once

struct msr_type;
class CreatureEntity;
void add_cheat_remove_flags_element(CreatureEntity &creature, msr_type *msr_ptr);
void check_element_resistance(msr_type *msr_ptr);
