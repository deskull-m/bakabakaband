#pragma once

class CreatureEntity;
int staff_effect(CreatureEntity &creature, int sval, bool *use_charge, bool powerful, bool magic, bool known);
void do_cmd_use_staff(CreatureEntity &creature);
