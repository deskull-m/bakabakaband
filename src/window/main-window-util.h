#pragma once

#include "system/angband.h"
#include "util/point-2d.h"
#include <string_view>

#define ROW_MAP 0
#define COL_MAP 12

class ItemEntity;
extern const ItemEntity *autopick_obj;
extern POSITION panel_row_min;
extern POSITION panel_row_max;
extern POSITION panel_col_min;
extern POSITION panel_col_max;
extern POSITION panel_col_prt;
extern POSITION panel_row_prt;
extern int match_autopick;
extern int feat_priority;

class DisplaySymbol;
class CreatureEntity;
void print_field(std::string_view info, TERM_LEN row, TERM_LEN col);
void print_map(CreatureEntity &creature);
void display_map(CreatureEntity &creature, int *cy, int *cx);
DisplaySymbol set_term_color(CreatureEntity &creature, const Pos2D &pos, const DisplaySymbol &symbol_orig);
int panel_col_of(int col);
