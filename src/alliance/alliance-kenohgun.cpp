/*!
 * @file alliance-kenohgun.cpp
 * @brief アライアンス「拳王軍」の実装
 */

#include "alliance/alliance-kenohgun.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
AllianceKenohgun::AllianceKenohgun(AllianceType id, std::string tag, std::string name, int64_t base_power)
    : Alliance(id, tag, name, base_power)
{
}

int AllianceKenohgun::calcImpressionPoint(const CreatureEntity &creature) const
{
    int impression = 0;
    impression += calcIronmanHostilityPenalty();

    impression += Alliance::calcPlayerPower(creature, 15, 20);
    return impression;
}

bool AllianceKenohgun::isAnnihilated()
{
    return MonraceList::get_instance().get_monrace(MonraceId::RAOU).mob_num == 0;
}
