#include "alliance/alliance-go.h"
#include "system/creature-entity.h"

int AllianceGO::calcImpressionPoint([[maybe_unused]] const CreatureEntity &creature) const
{
    int impression = 0;
    impression += calcIronmanHostilityPenalty();

    return impression;
}
