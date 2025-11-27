#include "alliance/alliance-ketholdeth.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"

#include "game-option/birth-options.h"
int AllianceKetholdeth::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    return 0;
}

bool AllianceKetholdeth::isAnnihilated()
{
    return MonraceList::get_instance().get_monrace(MonraceId::PRINCESS_KETHOLDETH).mob_num == 0;
}
