#pragma once

#include "system/angband.h"

class CreatureEntity;
void move_cursor_relative(int row, int col);
void print_path(CreatureEntity &creature, POSITION y, POSITION x);
bool change_panel(CreatureEntity &creature, POSITION dy, POSITION dx);
void panel_bounds_center(void);
