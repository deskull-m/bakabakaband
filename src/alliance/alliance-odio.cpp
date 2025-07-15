#include "alliance/alliance-odio.h"
#include "alliance/alliance.h"
#include "effect/effect-characteristics.h"
#include "floor/floor-util.h"
#include "monster-floor/monster-summon.h"
#include "monster-race/race-flags1.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

int AllianceOdio::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    return 0;
}
