#include "alliance/alliance-phyrexia.h"
#include "system/player-type-definition.h"

#include "game-option/birth-options.h"
int AlliancePhyrexia::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    // 鉄人モード: 全てのアライアンスから猛烈に敵対される
    if (ironman_alliance_hostility) {
        impression -= 10000;
    }

    impression += Alliance::calcPlayerPower(*creature_ptr, 15, 21);
    return impression;
}
