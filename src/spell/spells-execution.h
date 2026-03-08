#pragma once

#include "spell/spells-util.h"
#include "system/angband.h"
#include <string>
#include <tl/optional.hpp>

enum class RealmType;
class CreatureEntity;
tl::optional<std::string> exe_spell(CreatureEntity &creature, RealmType realm, SPELL_IDX spell, SpellProcessType mode);
