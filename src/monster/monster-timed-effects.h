#pragma once

#include "system/creature-timed-effect-types.h"
#include <array>
#include <map>
#include <string>

/*!
 * @brief モンスターが持ちうる時限効果の一覧
 * @details CreatureTimedEffect のうちモンスター側で扱う 7 種。
 * プレイヤー専用効果（HERO/BLESSED 等）はここに含まれない。
 */
constexpr std::array<CreatureTimedEffect, 7> MONSTER_TIMED_EFFECT_LIST = { {
    CreatureTimedEffect::SLEEP_OR_PARALYSIS,
    CreatureTimedEffect::ACCELERATION,
    CreatureTimedEffect::DECELERATION,
    CreatureTimedEffect::STUN,
    CreatureTimedEffect::CONFUSION,
    CreatureTimedEffect::FEAR,
    CreatureTimedEffect::INVULNERABILITY,
} };

extern const std::map<CreatureTimedEffect, std::string> effect_type_to_label;
