#include "alliance/alliance-aryan-family.h"
#include "system/player-type-definition.h"

int AllianceAryanFamily::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 14, 22);
    return impression;
}
