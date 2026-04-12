#include "alliance/alliance-chinchintei.h"
#include "alliance/alliance.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-list.h"

int AllianceChinChinTei::calcImpressionPoint([[maybe_unused]] const CreatureEntity &creature) const
{
    int impression = 0;
    impression += calcIronmanHostilityPenalty();

    return impression;
}

bool AllianceChinChinTei::isAnnihilated()
{
    return false;
}
