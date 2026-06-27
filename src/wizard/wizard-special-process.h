#pragma once

#include "system/angband.h"

class CreatureEntity;
void wiz_cure_all(CreatureEntity &creature);
void wiz_create_item(CreatureEntity &creature);
void wiz_create_named_art(CreatureEntity &creature);
void wiz_change_status(CreatureEntity &creature);
void wiz_create_feature(CreatureEntity &creature);
void wiz_jump_to_dungeon(CreatureEntity &creature);
void wiz_learn_items_all(CreatureEntity &creature);
void wiz_reset_race(CreatureEntity &creature);
void wiz_reset_class(CreatureEntity &creature);
void wiz_reset_realms(CreatureEntity &creature);
void wiz_dump_options();
void wiz_dump_current_floor(CreatureEntity &creature);
void wiz_zap_surrounding_monsters(CreatureEntity &creature);
void wiz_zap_floor_monsters(CreatureEntity &creature);
void wiz_level_up_target_monster(CreatureEntity &creature);
void cheat_death(CreatureEntity &creature, bool no_penalty);
