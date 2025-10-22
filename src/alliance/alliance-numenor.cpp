#include "alliance/alliance-numenor.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"

int AllianceNumenor::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 10, 20);
    return impression;
}

/*!
 * @brief ヌメノールのアライアンスが壊滅したかどうか判定する
 * @return 壊滅フラグ
 * @details 黄金王『アル=ファラゾン』が存在しない場合に壊滅する
 */
bool AllianceNumenor::isAnnihilated()
{
    // 黄金王『アル=ファラゾン』が存在しない場合、ヌメノールは壊滅する
    return MonraceList::get_instance().get_monrace(MonraceId::AR_PHARAZON).mob_num == 0;
}
