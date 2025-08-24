#include "alliance/alliance-mabelode.h"
#include "system/player-type-definition.h"

int AllianceMabelode::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 13, 26);
    return impression;
}
