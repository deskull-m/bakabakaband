#pragma once

#include "system/angband.h"

class CreatureEntity;
void redraw_window();
void window_stuff(CreatureEntity &creature);
void window_stuff_including_deferred(CreatureEntity &creature);
void redraw_stuff(CreatureEntity &creature);
