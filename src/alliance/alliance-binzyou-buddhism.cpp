#include "alliance/alliance-binzyou-buddhism.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"

int AllianceBinzyouBuddhism::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    return 0;
}

bool AllianceBinzyouBuddhism::isAnnihilated()
{
    return MonraceList::get_instance().get_monrace(MonraceId::BINZYOU_MUR).mob_num == 0;
}
