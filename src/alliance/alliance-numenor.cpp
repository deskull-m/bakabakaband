#include "alliance/alliance-numenor.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"

#include "game-option/birth-options.h"
int AllianceNumenor::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    // 鉄人モード: 全てのアライアンスから猛烈に敵対される
    if (ironman_alliance_hostility) {
        impression -= 10000;
    }

    impression += Alliance::calcPlayerPower(*creature_ptr, 10, 20);

    // ヌメノール・アライアンスに属するモンスターの撃破による印象値減少
    const auto &monrace_list = MonraceList::get_instance();

    // 黄金王『アル=ファラゾン』の撃破による大幅減点
    impression -= monrace_list.get_monrace(MonraceId::AR_PHARAZON).r_akills * 700;

    // ヌメノール装甲歩兵の撃破による減点
    impression -= monrace_list.get_monrace(MonraceId::NUMENOR_INFANTRY).r_akills * 10;

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
