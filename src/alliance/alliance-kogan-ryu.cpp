#include "alliance/alliance-kogan-ryu.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
int AllianceKoganRyu::calcImpressionPoint(const CreatureEntity &creature) const
{
    int impression = 0;
    impression += calcIronmanHostilityPenalty();

    impression += Alliance::calcPlayerPower(creature, 15, 18);
    return impression;
}

/*!
 * @brief 虎眼流のアライアンスが壊滅したかどうか判定する
 * @return 壊滅フラグ
 * @details 濃尾無双『岩本虎眼』が存在しない場合に壊滅する
 */
bool AllianceKoganRyu::isAnnihilated()
{
    // 濃尾無双『岩本虎眼』が存在しない場合、虎眼流は壊滅する
    return MonraceList::get_instance().get_monrace(MonraceId::IWAMOTO_KOGAN).mob_num == 0;
}
