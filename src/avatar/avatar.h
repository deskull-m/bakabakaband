#pragma once

#include "system/angband.h"
#include <map>
#include <string>

enum class Virtue : short {
    NONE = 0,
    COMPASSION = 1,
    HONOUR = 2,
    JUSTICE = 3,
    SACRIFICE = 4,
    KNOWLEDGE = 5,
    FAITH = 6,
    ENLIGHTEN = 7,
    ENCHANT = 8,
    CHANCE = 9,
    NATURE = 10,
    HARMONY = 11,
    VITALITY = 12,
    UNLIFE = 13,
    PATIENCE = 14,
    TEMPERANCE = 15,
    DILIGENCE = 16,
    VALOUR = 17,
    INDIVIDUALISM = 18,
    MAX,
};

class CreatureEntity;
class PlayerType;
extern const std::map<Virtue, std::string> virtue_names;
void initialize_virtues(CreatureEntity &creature);
bool compare_virtue(CreatureEntity &creature, Virtue virtue, int threshold);
int virtue_number(CreatureEntity &creature, Virtue virtue);
void chg_virtue(CreatureEntity &creature, Virtue virtue, int amount);
void set_virtue(CreatureEntity &creature, Virtue virtue, int amount);
void dump_virtues(CreatureEntity &creature, FILE *out_file);
