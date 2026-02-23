#pragma once

struct msr_type;
class CreatureEntity;
void add_cheat_remove_flags_others(CreatureEntity &creature, msr_type *msr_ptr);
void check_high_resistances(CreatureEntity &creature, msr_type *msr_ptr);
