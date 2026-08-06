#pragma once

class Direction;
class CreatureEntity;
void wild_magic(CreatureEntity &creature, int spell);
void call_chaos(CreatureEntity &creature);
bool activate_ty_curse(CreatureEntity &creature, bool stop_ty, int *count);
void cast_wonder(CreatureEntity &creature, const Direction &dir);
