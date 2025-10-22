#include "alliance/alliance-amber.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"

int AllianceAmber::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 10, 35);
    return impression;
}

/*!
 * @brief アンバーのアライアンスが壊滅したかどうか判定する
 * @return 壊滅フラグ
 * @details アンバーの王『オベロン』が存在しない場合に壊滅する
 */
bool AllianceAmber::isAnnihilated()
{
    // アンバーの王『オベロン』が存在しない場合、アンバーは壊滅する
    // return MonraceList::get_instance().get_monrace(MonraceId::OBERON).mob_num == 0;
    return false;
}
