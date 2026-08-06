#pragma once

struct lore_type;
class CreatureEntity;
void display_monster_breath(lore_type *lore_ptr);
void display_monster_magic_types(lore_type *lore_ptr);
void display_mosnter_magic_possibility(lore_type *lore_ptr);
void display_monster_magic_rate(lore_type *lore_ptr);
void display_monster_magic_tables(CreatureEntity &creature, lore_type *lore_ptr);
