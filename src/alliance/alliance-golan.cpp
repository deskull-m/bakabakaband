#include "alliance/alliance-golan.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"

int AllianceGOLAN::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;

    impression += Alliance::calcPlayerPower(*creature_ptr, 15, 12);
    impression -= MonraceList::get_instance().get_monrace(MonraceId::GOLAN_COLONEL).r_pkills * 1200;
    impression -= MonraceList::get_instance().get_monrace(MonraceId::GOLAN_MAD).r_pkills * 100;
    impression -= MonraceList::get_instance().get_monrace(MonraceId::GOLAN_RED_BELET).r_pkills * 60;
    impression -= MonraceList::get_instance().get_monrace(MonraceId::GOLAN_OFFICER).r_pkills * 10;
    impression -= MonraceList::get_instance().get_monrace(MonraceId::GOLAN_SOLDIER).r_pkills * 2;
    return impression;
}
