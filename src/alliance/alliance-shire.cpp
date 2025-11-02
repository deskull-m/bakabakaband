#include "alliance/alliance-shire.h"
#include "alliance/alliance.h"
#include "effect/effect-characteristics.h"
#include "floor/floor-util.h"
#include "monster-floor/monster-summon.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

int AllianceTheShire::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, -10, 1);

    // THE-SHIRE所属モンスターの殺害による印象値減少
    const std::string kill_key = "KILL_ALLIANCE/" + std::to_string(static_cast<int>(AllianceType::THE_SHIRE));
    if (creature_ptr->incident_tree.count(kill_key)) {
        impression -= creature_ptr->incident_tree.at(kill_key) * 10; // 1体につき-10
    }

    // 邪悪なモンスター殺害による印象値増加（EVIL = 11）
    const std::string evil_kill_key = "KILL/EVIL";
    if (creature_ptr->incident_tree.count(evil_kill_key)) {
        impression += creature_ptr->incident_tree.at(evil_kill_key) * 1; // 1体につき+1
    }

    // 善良なモンスター殺害による印象値減少（GOOD = 12）
    const std::string good_kill_key = "KILL/GOOD";
    if (creature_ptr->incident_tree.count(good_kill_key)) {
        impression -= creature_ptr->incident_tree.at(good_kill_key) * 2; // 1体につき-2
    }

    return impression;
}
