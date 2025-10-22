#include "alliance/alliance-ge-orlic.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"

int AllianceGEOrlic::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 10, 30);
    return impression;
}

/*!
 * @brief オーリック朝銀河帝国のアライアンスが壊滅したかどうか判定する
 * @return 壊滅フラグ
 * @details 銀河皇帝『カル・ダームIII世』が存在しない場合に壊滅する
 */
bool AllianceGEOrlic::isAnnihilated()
{
    // 銀河皇帝『カル・ダームIII世』が存在しない場合、オーリック朝銀河帝国は壊滅する
    return MonraceList::get_instance().get_monrace(MonraceId::CALDARM).mob_num == 0;
}
