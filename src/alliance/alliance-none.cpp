#include "alliance/alliance-none.h"
#include "system/player-type-definition.h"

int AllianceNone::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += calcIronmanHostilityPenalty();

    return impression;
}
