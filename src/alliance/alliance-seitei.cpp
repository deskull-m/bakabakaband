#include "alliance/alliance-seitei.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"

#include "game-option/birth-options.h"
int AllianceSEITEI::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    // 鉄人モード: 全てのアライアンスから猛烈に敵対される
    if (ironman_alliance_hostility) {
        impression -= 10000;
    }

    impression += Alliance::calcPlayerPower(*creature_ptr, 15, 15);

    // 聖帝軍アライアンスのモンスター撃破による印象値低下
    // TODO: 聖帝軍関連モンスター実装時に追加

    return impression;
}

bool AllianceSEITEI::isAnnihilated()
{
    // TODO: 聖帝サウザー実装時に条件を追加
    return false;
}