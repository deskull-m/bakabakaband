#include "alliance/alliance-basam-empire.h"
#include "system/player-type-definition.h"

int AllianceBasamEmpire::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 13, 24);
    return impression;
}
