#include "alliance/alliance-valinor.h"
#include "alliance/alliance.h"
#include "core/disturbance.h"
#include "effect/effect-characteristics.h"
#include "floor/floor-util.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/one-monster-placer.h"
#include "monster-floor/place-monster-types.h"
#include "spell/summon-types.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
int AllianceValinor::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += (creature_ptr->alignment > 0) ? creature_ptr->alignment : -creature_ptr->alignment * 3;
    impression += Alliance::calcPlayerPower(*creature_ptr, -16, 30);
    return impression;
}
