#pragma once

class CreatureEntity;
bool inc_stat(CreatureEntity &creature, int stat);
bool dec_stat(CreatureEntity &creature, int stat, int amount, int permanent);
bool res_stat(CreatureEntity &creature, int stat);
bool do_dec_stat(CreatureEntity &creature, int stat);
bool do_res_stat(CreatureEntity &creature, int stat);
bool do_inc_stat(CreatureEntity &creature, int stat);
bool lose_all_info(CreatureEntity &creature);
