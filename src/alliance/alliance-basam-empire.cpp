#include "alliance/alliance-basam-empire.h"
#include "game-option/birth-options.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"

int AllianceBasamEmpire::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 13, 24);
    // 鉄人モード: 全てのアライアンスから猛烈に敵対される
    if (ironman_alliance_hostility) {
        impression -= 10000;
    }

    return impression;
}

/*!
 * @brief バサム帝国のアライアンスが壊滅したかどうか判定する
 * @return 壊滅フラグ
 * @details 『オゴレス王』が存在しない場合に壊滅する
 */
bool AllianceBasamEmpire::isAnnihilated()
{
    // 『オゴレス王』が存在しない場合、バサム帝国は壊滅する
    return MonraceList::get_instance().get_monrace(MonraceId::OGRES_KING).mob_num == 0;
}
