#include "alliance/alliance-xiombarg.h"
#include "system/player-type-definition.h"

int AllianceXiombarg::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 15, 28);
    impression += calcIronmanHostilityPenalty();

    return impression;
}
