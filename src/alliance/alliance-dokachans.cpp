#include "alliance/alliance-dokachans.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"

int AllianceDokachans::calcImpressionPoint([[maybe_unused]] const CreatureEntity &creature) const
{
    auto impression = 0;
    impression += calcIronmanHostilityPenalty();

    if (MonraceList::get_instance().get_monrace(MonraceId::DOKACHAN).mob_num == 0) {
        impression -= 1000;
    }
    if (MonraceList::get_instance().get_monrace(MonraceId::OLDMAN_60TY).mob_num == 0) {
        impression -= 1000;
    }
    if (MonraceList::get_instance().get_monrace(MonraceId::BROTHER_45TH).mob_num == 0) {
        impression -= 1000;
    }
    return impression;
}

bool AllianceDokachans::isAnnihilated()
{
    return all_monraces_extinct({ MonraceId::DOKACHAN });
}
