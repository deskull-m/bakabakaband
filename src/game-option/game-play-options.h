#pragma once

#include "system/angband.h"

extern bool stack_force_notes; /* Merge inscriptions when stacking */
extern bool stack_force_costs; /* Merge discounts when stacking */
extern bool expand_list; /* Expand the power of the list commands */
extern bool allow_smallest_floor; /* Allow unusually small dungeon levels */
extern bool always_small_floor; /* Always create unusually small dungeon levels */
extern bool allow_largest_floor; //!< 最大面積のフロアを他ダンジョンでも生成可能にする.
extern bool always_large_floor; //!< 常に大きいフロアを生成する.
extern bool allow_arena_floor; /* Allow empty 'on_defeat_arena_monster' levels */
extern bool bound_walls_perm; /* Boundary walls become 'permanent wall' */
extern bool last_words; /* Leave last words when your character dies */
extern bool monster_tombstones; /* Erect tombstones for all dead monsters (joke option) */
extern bool auto_dump; /* Dump a character record automatically */
extern bool send_score; /* Send score dump to the world score server */
extern bool allow_debug_opts; /* Allow use of debug/cheat options */
