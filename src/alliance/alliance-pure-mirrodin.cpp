#include "alliance/alliance-pure-mirrodin.h"
#include "system/player-type-definition.h"
int AlliancePureMirrodin::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += calcIronmanHostilityPenalty();

    impression += Alliance::calcPlayerPower(*creature_ptr, 10, 12);
    return impression;
}
