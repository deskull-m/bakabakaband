#include "alliance/alliance-go.h"
#include "system/creature-entity.h"
#include "system/player-type-definition.h"

int AllianceGO::calcImpressionPoint([[maybe_unused]] const CreatureEntity &creature) const
{
    int impression = 0;
    impression += calcIronmanHostilityPenalty();

    return impression;
}
