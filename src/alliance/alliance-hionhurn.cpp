#include "alliance/alliance-hionhurn.h"
#include "system/player-type-definition.h"

int AllianceHionhurn::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 12, 25);
    return impression;
}
