#include "alliance/alliance-avarin-lords.h"
#include "system/player-type-definition.h"

int AllianceAvarinLords::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 10, 23);
    return impression;
}
