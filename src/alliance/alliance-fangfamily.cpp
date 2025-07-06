#include "alliance/alliance-fangfamily.h"
#include "alliance/alliance.h"
#include "core/disturbance.h"
#include "effect/effect-characteristics.h"
#include "floor/floor-util.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/one-monster-placer.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/race-flags1.h"
#include "spell/summon-types.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

int AllianceFangFamily::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 5, 10);
    impression -= MonraceList::get_instance().get_monrace(MonraceId::FANG_FAMILY).r_akills * 5;
    if (MonraceList::get_instance().get_monrace(MonraceId::KING_FANG_FAMILY).mob_num == 0) {
        impression -= 300;
    }
    return impression;
}

bool AllianceFangFamily::isAnnihilated()
{
    return MonraceList::get_instance().get_monrace(MonraceId::KING_FANG_FAMILY).mob_num == 0;
}
