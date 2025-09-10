#include "alliance/alliance-king.h"
#include "system/player-type-definition.h"

int AllianceKING::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 15, 15);
    return impression;
}
