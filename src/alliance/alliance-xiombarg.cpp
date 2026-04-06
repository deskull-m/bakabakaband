#include "alliance/alliance-xiombarg.h"
#include "system/creature-entity.h"
#include "system/player-type-definition.h"

int AllianceXiombarg::calcImpressionPoint(const CreatureEntity &creature) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(creature, 15, 28);
    impression += calcIronmanHostilityPenalty();

    return impression;
}
