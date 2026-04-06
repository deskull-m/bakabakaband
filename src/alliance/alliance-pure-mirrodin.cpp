#include "alliance/alliance-pure-mirrodin.h"
#include "system/creature-entity.h"
#include "system/player-type-definition.h"
int AlliancePureMirrodin::calcImpressionPoint([[maybe_unused]] const CreatureEntity &creature) const
{
    int impression = 0;
    impression += calcIronmanHostilityPenalty();

    impression += Alliance::calcPlayerPower(creature, 10, 12);
    return impression;
}
