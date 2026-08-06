#include "alliance/alliance-tophamhatt.h"
#include "alliance/alliance.h"
#include "effect/effect-characteristics.h"
#include "floor/floor-util.h"
#include "monster-floor/monster-summon.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

int AllianceTophamHatt::calcImpressionPoint(const CreatureEntity &creature) const
{
    auto impression = Alliance::calcPlayerPower(creature, 10, 5);

    const auto &monraces = MonraceList::get_instance();

    // 人面機関車 (レベル52)
    impression -= monraces.get_monrace(MonraceId::HUMAN_FACE_LOCOMOTIVE).r_pkills * 50;

    // トップハム・ハット一族の技師 (レベル55)
    impression -= monraces.get_monrace(MonraceId::TOPHAMHATT_ENGINEER).r_pkills * 550;

    return impression;
}

bool AllianceTophamHatt::isAnnihilated()
{
    return all_monraces_extinct({ MonraceId::TOPHAMHATT_ENGINEER });
}
