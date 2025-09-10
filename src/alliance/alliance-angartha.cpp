#include "alliance/alliance-angartha.h"
#include "system/player-type-definition.h"

int AllianceAngartha::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 11, 20);
    return impression;
}
