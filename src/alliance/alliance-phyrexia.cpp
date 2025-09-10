#include "alliance/alliance-phyrexia.h"
#include "system/player-type-definition.h"

int AlliancePhyrexia::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 15, 21);
    return impression;
}
