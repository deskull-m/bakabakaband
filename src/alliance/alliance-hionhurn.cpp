#include "alliance/alliance-hionhurn.h"
#include "system/creature-entity.h"

int AllianceHionhurn::calcImpressionPoint(const CreatureEntity &creature) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(creature, 12, 25);
    impression += calcIronmanHostilityPenalty();

    return impression;
}
