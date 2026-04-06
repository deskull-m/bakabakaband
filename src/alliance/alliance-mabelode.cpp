#include "alliance/alliance-mabelode.h"
#include "system/creature-entity.h"
#include "system/player-type-definition.h"
int AllianceMabelode::calcImpressionPoint(const CreatureEntity &creature) const
{
    int impression = 0;
    impression += calcIronmanHostilityPenalty();

    impression += Alliance::calcPlayerPower(creature, 13, 26);
    return impression;
}
