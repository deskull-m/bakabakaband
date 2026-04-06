#include "alliance/alliance-getter.h"
#include "system/creature-entity.h"
#include "system/player-type-definition.h"

int AllianceGetter::calcImpressionPoint([[maybe_unused]] const CreatureEntity &creature) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(creature, 20, 1);
    impression += calcIronmanHostilityPenalty();

    return impression;
}
