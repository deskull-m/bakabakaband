#pragma once

#include "dungeon/quest.h"
#include <optional>

class PlayerType;
void play_game(PlayerType *player_ptr, bool new_game, bool browsing_movie, std::optional<QuestId> initial_quest_id = std::nullopt);
