#pragma once

class CreatureEntity;
void do_cmd_go_up(CreatureEntity &creature);
void do_cmd_go_down(CreatureEntity &creature);
void do_cmd_go_portal(CreatureEntity &creature);
void do_cmd_walk(CreatureEntity &creature, bool pickup);
void do_cmd_run(CreatureEntity &creature);
void do_cmd_stay(CreatureEntity &creature, bool pickup);
void do_cmd_rest(CreatureEntity &creature);
