#include "alliance/alliance-getter.h"
#include "system/player-type-definition.h"

int AllianceGetter::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 20, 1);
    return impression;
}
