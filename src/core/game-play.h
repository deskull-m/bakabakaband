#pragma once

#include "dungeon/quest.h"
#include <optional>

class CreatureEntity;
void play_game(CreatureEntity &creature, bool new_game, bool browsing_movie, std::optional<QuestId> initial_quest_id = std::nullopt);
