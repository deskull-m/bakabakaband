#pragma once

#include <string_view>

enum class BirthKind {
    REALM,
    RACE,
    CLASS,
    PERSONALITY,
    PATRON,
    AUTO_ROLLER,
};

class CreatureEntity;
class PlayerType;
void birth_quit();
void show_help(CreatureEntity &creature, std::string_view helpfile);
void birth_help_option(CreatureEntity &creature, char c, BirthKind bk);
