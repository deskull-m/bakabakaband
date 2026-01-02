#include "alliance/alliance-go.h"
#include "system/player-type-definition.h"

int AllianceGO::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += calcIronmanHostilityPenalty();

    return impression;
}
