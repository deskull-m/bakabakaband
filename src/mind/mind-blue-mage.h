#pragma once

#include <array>

enum class BlueMagicType {
    BOLT = 1,
    BALL = 2,
    BREATH = 3,
    SUMMON = 4,
    OTHER = 5,
};

class CreatureEntity;
bool do_cmd_cast_learned(CreatureEntity &creature);

inline constexpr std::array<BlueMagicType, 5> BLUE_MAGIC_TYPE_LIST = { {
    BlueMagicType::BOLT,
    BlueMagicType::BALL,
    BlueMagicType::BREATH,
    BlueMagicType::SUMMON,
    BlueMagicType::OTHER,
} };
