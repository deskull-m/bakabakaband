#pragma once

class CreatureEntity;
void do_cmd_save_game(CreatureEntity &creature, int is_autosave);
void do_cmd_save_and_exit(CreatureEntity &creature);
void request_auto_save();
void do_requested_auto_save(CreatureEntity &creature);
