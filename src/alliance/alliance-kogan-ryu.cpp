#include "alliance/alliance-kogan-ryu.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"

#include "game-option/birth-options.h"
int AllianceKoganRyu::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    // 鉄人モード: 全てのアライアンスから猛烈に敵対される
    if (ironman_alliance_hostility) {
        impression -= 10000;
    }

    impression += Alliance::calcPlayerPower(*creature_ptr, 15, 18);
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
