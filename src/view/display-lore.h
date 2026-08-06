#pragma once

#include "lore/lore-util.h"
#include "system/angband.h"

struct lore_type;
class PlayerType;
class CreatureEntity;
void roff_top(MonraceId monrace_id);
void screen_roff(CreatureEntity &creature, MonraceId r_idx, monster_lore_mode mode);
void display_roff(CreatureEntity &creature);
void output_monster_spoiler(MonraceId r_idx, hook_c_roff_pf roff_func);
void display_kill_numbers(lore_type *lore_ptr);
void display_where_to_appear_summary(lore_type *lore_ptr);
bool display_where_to_appear(lore_type *lore_ptr);
void display_monster_speed_summary(lore_type *lore_ptr);
void display_monster_move(lore_type *lore_ptr);
void display_monster_never_move(lore_type *lore_ptr);
void display_monster_exp_summary(lore_type *lore_ptr);
void display_monster_kills_summary(lore_type *lore_ptr);
void display_monster_kind_tags(lore_type *lore_ptr);
void display_monster_kind(lore_type *lore_ptr);
void display_monster_alignment(lore_type *lore_ptr);
void display_monster_exp(CreatureEntity &creature, lore_type *lore_ptr);
void set_monster_aura_summary(lore_type *lore_ptr);
void display_monster_aura(lore_type *lore_ptr);
void display_lore_this(CreatureEntity &creature, lore_type *lore_ptr);
void display_monster_collective(lore_type *lore_ptr);
void display_monster_launching(CreatureEntity &creature, lore_type *lore_ptr);
void display_monster_sometimes(lore_type *lore_ptr);
void display_monster_dead_spawns(lore_type *lore_ptr);
void display_drop_kind_items(lore_type *lore_ptr);
void display_monster_guardian(lore_type *lore_ptr);
