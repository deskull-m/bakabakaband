#pragma once

struct lore_type;
class CreatureEntity;
void set_breath_types(CreatureEntity &creature, lore_type *lore_ptr);
void set_ball_types(CreatureEntity &creature, lore_type *lore_ptr);
void set_particular_types(CreatureEntity &creature, lore_type *lore_ptr);
void set_bolt_types(CreatureEntity &creature, lore_type *lore_ptr);
void set_status_types(lore_type *lore_ptr);
void set_teleport_types(lore_type *lore_ptr);
void set_floor_types(CreatureEntity &creature, lore_type *lore_ptr);
void set_summon_types(lore_type *lore_ptr);
