#include "alliance/alliance-megadeth.h"
#include "alliance/alliance.h"
#include "effect/effect-characteristics.h"
#include "floor/floor-util.h"
#include "monster-floor/monster-summon.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

#include "game-option/birth-options.h"
int AllianceMegadeth::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    // 鉄人モード: 全てのアライアンスから猛烈に敵対される
    if (ironman_alliance_hostility) {
        impression -= 10000;
    }

    impression += Alliance::calcPlayerPower(*creature_ptr, 5, 10);
    return impression;
}

bool AllianceMegadeth::isAnnihilated()
{
    return false;
}
