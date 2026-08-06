#include "alliance/alliance-ketholdeth.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"

int AllianceKetholdeth::calcImpressionPoint([[maybe_unused]] const CreatureEntity &creature) const
{
    return 0;
}

bool AllianceKetholdeth::isAnnihilated()
{
    return all_monraces_extinct({ MonraceId::PRINCESS_KETHOLDETH });
}
