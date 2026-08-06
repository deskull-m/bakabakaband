#pragma once

class CreatureEntity;
class FloorType;
void wipe_generate_random_floor_flags(FloorType &floor);
void apply_terrain_generation_changes(FloorType &floor);
void clear_cave(CreatureEntity &creature);
void generate_floor(CreatureEntity &creature);
