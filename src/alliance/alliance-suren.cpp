#include "alliance/alliance-suren.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"

int AllianceSuren::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    return 0;
}

bool AllianceSuren::isAnnihilated()
{
    return MonraceList::get_instance().get_monrace(MonraceId::SUREN).mob_num == 0;
}
