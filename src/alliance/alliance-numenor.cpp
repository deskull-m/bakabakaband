#include "alliance/alliance-numenor.h"
#include "system/player-type-definition.h"

int AllianceNumenor::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 10, 20);
    return impression;
}
