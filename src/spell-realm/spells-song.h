#pragma once

#include "system/angband.h"

class CreatureEntity;
void check_music(CreatureEntity &creature);
bool set_tim_stealth(CreatureEntity &creature, TIME_EFFECT v, bool do_dec);
void stop_singing(CreatureEntity &creature);
bool music_singing(CreatureEntity &creature, int music_songs);
bool music_singing_any(CreatureEntity &creature);
int32_t get_singing_song_effect(CreatureEntity &creature);
void set_singing_song_effect(CreatureEntity &creature, const int32_t magic_num);
int32_t get_interrupting_song_effect(CreatureEntity &creature);
void set_interrupting_song_effect(CreatureEntity &creature, const int32_t magic_num);
int32_t get_singing_count(CreatureEntity &creature);
void set_singing_count(CreatureEntity &creature, const int32_t magic_num);
byte get_singing_song_id(CreatureEntity &creature);
void set_singing_song_id(CreatureEntity &creature, const byte magic_num);
