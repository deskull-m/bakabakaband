#pragma once

#include "spell/spells-util.h"
#include "system/angband.h"
#include <string>
#include <tl/optional.hpp>

class CreatureEntity;
tl::optional<std::string> do_life_spell(CreatureEntity &creature, SPELL_IDX spell, SpellProcessType mode);
