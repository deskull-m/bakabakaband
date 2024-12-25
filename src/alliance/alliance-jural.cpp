#include "alliance/alliance-jural.h"
#include "alliance/alliance.h"
#include "effect/effect-characteristics.h"
#include "floor/floor-util.h"
#include "monster-floor/monster-summon.h"
#include "monster-race/race-flags1.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

int AllianceJural::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 5, 10);
    impression -= monraces_info[MonsterRaceId::ALIEN_JURAL].r_akills * 5;
    if (monraces_info[MonsterRaceId::JURAL_MONS].mob_num == 0) {
        impression -= 300;
    }
    if (monraces_info[MonsterRaceId::JURAL_WITCHKING].mob_num == 0) {
        impression -= 1230;
    }
    return impression;
}

bool AllianceJural::isAnnihilated()
{
    return monraces_info[MonsterRaceId::JURAL_WITCHKING].mob_num == 0;
}
