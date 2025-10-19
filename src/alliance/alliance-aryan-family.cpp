#include "alliance/alliance-aryan-family.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"

int AllianceAryanFamily::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 14, 22);
    return impression;
}

/*!
 * @brief アーリアンファミリーのアライアンスが壊滅したかどうか判定する
 * @return 壊滅フラグ
 * @details アーリアン・ファミリーのボス『ビッグ・アイ』が存在しない場合に壊滅する
 */
bool AllianceAryanFamily::isAnnihilated()
{
    // アーリアン・ファミリーのボス『ビッグ・アイ』が存在しない場合、アーリアンファミリーは壊滅する
    return MonraceList::get_instance().get_monrace(MonraceId::BIG_EYE).cur_num == 0;
}
