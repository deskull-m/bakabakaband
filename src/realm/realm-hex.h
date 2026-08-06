#pragma once

#include "realm/realm-hex-numbers.h"
#include "spell/spells-util.h"
#include "system/angband.h"
#include <string>
#include <tl/optional.hpp>

class CreatureEntity;
tl::optional<std::string> do_hex_spell(CreatureEntity &creature, spell_hex_type spell, SpellProcessType mode);
