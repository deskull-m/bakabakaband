#include "alliance/alliance-kogan-ryu.h"
#include "system/player-type-definition.h"

int AllianceKoganRyu::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 15, 18);
    return impression;
}
