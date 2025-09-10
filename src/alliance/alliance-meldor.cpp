#include "alliance/alliance-meldor.h"
#include "system/player-type-definition.h"

int AllianceMeldor::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 13, 28);
    return impression;
}
