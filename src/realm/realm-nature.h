#pragma once

#include "spell/spells-util.h"
#include "system/angband.h"
#include <string>
#include <tl/optional.hpp>

class PlayerType;
tl::optional<std::string> do_nature_spell(PlayerType *player_ptr, SPELL_IDX spell, SpellProcessType mode);
