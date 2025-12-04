#include "alliance/alliance-binzyou-buddhism.h"
#include "game-option/birth-options.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"

int AllianceBinzyouBuddhism::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int result = 0;
    // 鉄人モード: 全てのアライアンスから猛烈に敵対される
    if (ironman_alliance_hostility) {
        result -= 10000;
    }

    const auto &monraces = MonraceList::get_instance();

    // 便乗仏教大僧正『MUR』 (レベル40)
    result -= monraces.get_monrace(MonraceId::BINZYOU_MUR).r_pkills * 400;

    // 便乗仏教修行僧 (レベル23)
    result -= monraces.get_monrace(MonraceId::BINZYOU_BUDDHISM_MONK).r_pkills * 10;

    return result;
}

bool AllianceBinzyouBuddhism::isAnnihilated()
{
    return MonraceList::get_instance().get_monrace(MonraceId::BINZYOU_MUR).mob_num == 0;
}
