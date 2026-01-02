#pragma once

#include "dungeon/quest.h"
#include <optional>

class PlayerType;
void player_birth(PlayerType *player_ptr, std::optional<QuestId> initial_quest_id = std::nullopt);
