#include "alliance/alliance-meldor.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"

int AllianceMeldor::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 13, 28);
    return impression;
}

bool AllianceMeldor::isAnnihilated()
{
    const auto &monrace_list = MonraceList::get_instance();
    return monrace_list.get_monrace(MonraceId::ANNATAR).cur_num == 0;
}
