#include "alliance/alliance-gaichi.h"
#include "system/player-type-definition.h"

int AllianceGaichi::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 4, 10);
    return impression;
}
