#include "alliance/alliance-arioch.h"
#include "system/player-type-definition.h"

int AllianceArioch::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 15, 27);
    impression += calcIronmanHostilityPenalty();
    return impression;
}
