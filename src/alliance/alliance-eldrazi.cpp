#include "alliance/alliance-eldrazi.h"
#include "game-option/birth-options.h"
#include "system/player-type-definition.h"

int AllianceEldrazi::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    // 鉄人モード: 全てのアライアンスから猛烈に敵対される
    if (ironman_alliance_hostility) {
        impression -= 10000;
    }

    return impression;
}
