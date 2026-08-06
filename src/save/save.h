#pragma once

enum class SaveType {
    CLOSE_GAME,
    CONTINUE_GAME
};

class CreatureEntity;
bool save_player(CreatureEntity &creature, SaveType type);
