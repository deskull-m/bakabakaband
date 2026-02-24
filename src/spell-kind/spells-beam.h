#pragma once

class CreatureEntity;
class Direction;
bool wall_to_mud(CreatureEntity &creature, const Direction &dir, int dam);
bool wizard_lock(CreatureEntity &creature, const Direction &dir);
bool destroy_door(CreatureEntity &creature, const Direction &dir);
bool disarm_trap(CreatureEntity &creature, const Direction &dir);
