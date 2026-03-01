#pragma once

class CreatureEntity;
class Direction;
int rod_effect(CreatureEntity &creature, int sval, const Direction &dir, bool *use_charge, bool powerful);
void do_cmd_zap_rod(CreatureEntity &creature);
