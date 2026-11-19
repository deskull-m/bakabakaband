#pragma once

#include "spell/spells-util.h"
#include "system/angband.h"

class CreatureEntity;
tl::optional<std::string> do_music_spell(CreatureEntity &creature, SPELL_IDX spell, SpellProcessType mode);
void keep_detection_song_marks(CreatureEntity &creature);
