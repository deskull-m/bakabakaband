#include "alliance/alliance-ge-orlic.h"
#include "system/player-type-definition.h"

int AllianceGEOrlic::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 10, 30);
    return impression;
}
