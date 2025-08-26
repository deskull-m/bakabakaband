#include "alliance/alliance-chinchintei.h"
#include "alliance/alliance.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"

int AllianceChinChinTei::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    return 0;
}

bool AllianceChinChinTei::isAnnihilated()
{
    return false;
}