#include "alliance/alliance-chardros.h"
#include "system/creature-entity.h"
#include "system/player-type-definition.h"

int AllianceChardros::calcImpressionPoint(const CreatureEntity &creature) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(creature, 14, 26);
    impression += calcIronmanHostilityPenalty();

    return impression;
}
