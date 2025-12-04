#include "alliance/alliance-go.h"
#include "game-option/birth-options.h"
#include "system/player-type-definition.h"

int AllianceGO::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    // 鉄人モード: 全てのアライアンスから猛烈に敵対される
    if (ironman_alliance_hostility) {
        impression -= 10000;
    }

    return impression;
}
