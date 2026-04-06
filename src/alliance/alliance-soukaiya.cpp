#include "alliance/alliance-soukaiya.h"
#include "system/creature-entity.h"
#include "effect/effect-characteristics.h"
#include "floor/floor-util.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "spell/summon-types.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

int AllianceSoukaiya::calcImpressionPoint([[maybe_unused]] const CreatureEntity &creature) const
{
    // プレイヤーレベルによる基本印象値
    auto impression = Alliance::calcPlayerPower(creature, 10, 5);

    return impression;
}

bool AllianceSoukaiya::isAnnihilated()
{
    // 主要メンバーが全滅していたら壊滅扱い
    // TODO: ソウカイヤの主要メンバーを設定

    return false;
}

void AllianceSoukaiya::panishment([[maybe_unused]] CreatureEntity &creature)
{
    // ソウカイヤの復讐処理
    // TODO: 適切な復讐モンスターを召喚

    return;
}
