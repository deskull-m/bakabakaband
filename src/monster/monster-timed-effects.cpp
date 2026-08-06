#include "monster/monster-timed-effects.h"
#include "locale/language-switcher.h"

const std::map<CreatureTimedEffect, std::string> effect_type_to_label = {
    { CreatureTimedEffect::SLEEP_OR_PARALYSIS, _("睡眠", "sleep") },
    { CreatureTimedEffect::ACCELERATION, _("加速", "haste") },
    { CreatureTimedEffect::DECELERATION, _("減速", "slow") },
    { CreatureTimedEffect::STUN, _("朦朧", "stun") },
    { CreatureTimedEffect::CONFUSION, _("混乱", "confuse") },
    { CreatureTimedEffect::FEAR, _("恐怖", "scare") },
    { CreatureTimedEffect::INVULNERABILITY, _("無敵", "invulnerable") },
};
