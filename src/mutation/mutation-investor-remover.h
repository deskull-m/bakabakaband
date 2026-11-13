#pragma once

#include "system/angband.h"
#include <tl/optional.hpp>

class CreatureEntity;
enum class PlayerMutationType;

bool gain_mutation(CreatureEntity &creature, tl::optional<PlayerMutationType> choose_mut = tl::nullopt);
bool lose_mutation(CreatureEntity &creature, tl::optional<PlayerMutationType> choose_mut = tl::nullopt);
void lose_all_mutations(CreatureEntity &creature);
