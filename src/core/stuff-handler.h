#pragma once

class CreatureEntity;
void handle_stuff(CreatureEntity &creature);
void health_track(CreatureEntity &creature, short m_idx);
bool update_player();
bool redraw_player(CreatureEntity &creature);
