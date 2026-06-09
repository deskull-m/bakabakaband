#include "game-option/game-play-options.h"

bool stack_force_notes; /* Merge inscriptions when stacking */
bool stack_force_costs; /* Merge discounts when stacking */
bool expand_list; /* Expand the power of the list commands */
bool allow_smallest_floor; /* Allow unusually small dungeon levels */
bool always_small_floor; /* Always create unusually small dungeon levels */
bool allow_largest_floor; /* Allow unusually large dungeon levels */
bool always_large_floor; /* Always create unusually large dungeon levels */
bool allow_arena_floor; /* Allow empty 'on_defeat_arena_monster' levels */
bool bound_walls_perm; /* Boundary walls become 'permanent wall' */
bool last_words; /* Leave last words when your character dies */
bool monster_tombstones; /* Erect tombstones for all dead monsters (joke option) */
bool auto_dump; /* Dump a character record automatically */
bool auto_debug_save; /* Dump a debug savedata every key input */
bool send_score; /* Send score dump to the world score server */
bool allow_debug_opts; /* Allow use of debug/cheat options */
