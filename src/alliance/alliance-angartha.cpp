#include "alliance/alliance-angartha.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"

int AllianceAngartha::calcImpressionPoint([[maybe_unused]] const CreatureEntity &creature) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(creature, 11, 20);
    impression += calcIronmanHostilityPenalty();

    return impression;
}

/*!
 * @brief アンガルサのアライアンスが壊滅したかどうか判定する
 * @return 壊滅フラグ
 * @details 万色の『サルマン』が存在しない場合に壊滅する
 */
bool AllianceAngartha::isAnnihilated()
{
    // 万色の『サルマン』が存在しない場合、アンガルサは壊滅する
    return MonraceList::get_instance().get_monrace(MonraceId::SARUMAN).mob_num == 0;
}
