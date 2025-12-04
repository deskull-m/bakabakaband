#include "alliance/alliance-king.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"

#include "game-option/birth-options.h"
int AllianceKING::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    // 鉄人モード: 全てのアライアンスから猛烈に敵対される
    if (ironman_alliance_hostility) {
        impression -= 10000;
    }

    impression += Alliance::calcPlayerPower(*creature_ptr, 15, 15);

    // KINGアライアンスのモンスター撃破による印象値低下
    impression -= MonraceList::get_instance().get_monrace(MonraceId::KING_SOLDIER).r_akills * 5;
    impression -= MonraceList::get_instance().get_monrace(MonraceId::KING_SPADE).r_akills * 50;
    impression -= MonraceList::get_instance().get_monrace(MonraceId::KING_DIA).r_akills * 50;
    impression -= MonraceList::get_instance().get_monrace(MonraceId::KING_CLUB).r_akills * 50;
    impression -= MonraceList::get_instance().get_monrace(MonraceId::MR_HEART).r_akills * 100;

    // KING『シン』が撃破されている場合の大幅減点
    if (MonraceList::get_instance().get_monrace(MonraceId::KING_SHIN).r_akills > 0) {
        impression -= 300;
    }

    return impression;
}

bool AllianceKING::isAnnihilated()
{
    return MonraceList::get_instance().get_monrace(MonraceId::KING_SHIN).mob_num == 0;
}
