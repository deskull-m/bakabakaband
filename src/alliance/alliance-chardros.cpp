#include "alliance/alliance-chardros.h"
#include "system/player-type-definition.h"

int AllianceChardros::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 14, 26);
    return impression;
}
