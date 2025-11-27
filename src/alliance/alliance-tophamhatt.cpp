#include "alliance/alliance-tophamhatt.h"
#include "alliance/alliance.h"
#include "effect/effect-characteristics.h"
#include "floor/floor-util.h"
#include "monster-floor/monster-summon.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

#include "game-option/birth-options.h"
int AllianceTophamHatt::calcImpressionPoint(PlayerType *creature_ptr) const
{
    auto impression = Alliance::calcPlayerPower(*creature_ptr, 10, 5);

    const auto &monraces = MonraceList::get_instance();

    // 人面機関車 (レベル52)
    impression -= monraces.get_monrace(MonraceId::HUMAN_FACE_LOCOMOTIVE).r_pkills * 50;

    // トップハム・ハット一族の技師 (レベル55)
    impression -= monraces.get_monrace(MonraceId::TOPHAMHATT_ENGINEER).r_pkills * 550;

    return impression;
}

bool AllianceTophamHatt::isAnnihilated()
{
    return MonraceList::get_instance().get_monrace(MonraceId::TOPHAMHATT_ENGINEER).mob_num == 0;
}
