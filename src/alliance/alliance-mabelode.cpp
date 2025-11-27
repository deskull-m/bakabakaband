#include "alliance/alliance-mabelode.h"
#include "system/player-type-definition.h"

#include "game-option/birth-options.h"
int AllianceMabelode::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    // 鉄人モード: 全てのアライアンスから猛烈に敵対される
    if (ironman_alliance_hostility) {
        impression -= 10000;
    }

    impression += Alliance::calcPlayerPower(*creature_ptr, 13, 26);
    return impression;
}
