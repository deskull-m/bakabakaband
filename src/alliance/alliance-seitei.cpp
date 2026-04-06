#include "alliance/alliance-seitei.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
int AllianceSEITEI::calcImpressionPoint([[maybe_unused]] const CreatureEntity &creature) const
{
    int impression = 0;
    impression += calcIronmanHostilityPenalty();

    impression += Alliance::calcPlayerPower(creature, 15, 15);

    // 聖帝軍アライアンスのモンスター撃破による印象値低下
    // TODO: 聖帝軍関連モンスター実装時に追加

    return impression;
}

bool AllianceSEITEI::isAnnihilated()
{
    // TODO: 聖帝サウザー実装時に条件を追加
    return false;
}
