#pragma once

#include "dungeon/quest.h"
#include <optional>

class CreatureEntity;
class PlayerType;
void player_birth(CreatureEntity &creature, std::optional<QuestId> initial_quest_id = std::nullopt);
