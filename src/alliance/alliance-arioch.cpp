#include "alliance/alliance-arioch.h"
#include "system/creature-entity.h"
#include "system/player-type-definition.h"

int AllianceArioch::calcImpressionPoint(const CreatureEntity &creature) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(creature, 15, 27);
    impression += calcIronmanHostilityPenalty();
    return impression;
}
