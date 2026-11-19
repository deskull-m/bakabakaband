#pragma once

class CreatureEntity;
void stop_running(CreatureEntity &creature);
void disturb(CreatureEntity &creature, bool stop_search, bool stop_travel);
void finish_timed_effect_change(CreatureEntity &creature);
