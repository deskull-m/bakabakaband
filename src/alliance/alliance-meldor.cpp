#include "alliance/alliance-meldor.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
int AllianceMeldor::calcImpressionPoint([[maybe_unused]] const CreatureEntity &creature) const
{
    int impression = 0;
    impression += calcIronmanHostilityPenalty();

    impression += Alliance::calcPlayerPower(creature, 13, 28);
    return impression;
}

bool AllianceMeldor::isAnnihilated()
{
    return all_monraces_extinct({ MonraceId::ANNATAR });
}
