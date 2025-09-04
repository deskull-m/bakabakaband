#include "alliance/alliance-amber.h"
#include "system/player-type-definition.h"

int AllianceAmber::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 10, 35);
    return impression;
}
