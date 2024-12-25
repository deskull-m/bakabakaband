#include "alliance/alliance-triothepunch.h"
#include "alliance/alliance.h"
#include "effect/effect-characteristics.h"
#include "floor/floor-util.h"
#include "monster-floor/monster-summon.h"
#include "monster-race/race-flags1.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

int AllianceTrioThePunch::calcImpressionPoint(PlayerType *creature_ptr) const
{
    auto impression = Alliance::calcPlayerPower(*creature_ptr, 10, 5);
    return impression;
}

bool AllianceTrioThePunch::isAnnihilated()
{
    return false;
}
