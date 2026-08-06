#pragma once

class CreatureEntity;
class Direction;
bool wand_effect(CreatureEntity &creature, int sval, const Direction &dir, bool powerful, bool magic);
void do_cmd_aim_wand(CreatureEntity &creature);
