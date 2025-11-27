#include "alliance/alliance-getter.h"
#include "game-option/birth-options.h"
#include "system/player-type-definition.h"

int AllianceGetter::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 20, 1);
    // 鉄人モード: 全てのアライアンスから猛烈に敵対される
    if (ironman_alliance_hostility) {
        impression -= 10000;
    }

    return impression;
}
