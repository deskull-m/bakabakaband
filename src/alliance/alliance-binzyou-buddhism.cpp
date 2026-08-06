#include "alliance/alliance-binzyou-buddhism.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"

int AllianceBinzyouBuddhism::calcImpressionPoint([[maybe_unused]] const CreatureEntity &creature) const
{
    int result = 0;
    result += calcIronmanHostilityPenalty();

    const auto &monraces = MonraceList::get_instance();

    // 便乗仏教大僧正『MUR』 (レベル40)
    result -= monraces.get_monrace(MonraceId::BINZYOU_MUR).r_pkills * 400;

    // 便乗仏教修行僧 (レベル23)
    result -= monraces.get_monrace(MonraceId::BINZYOU_BUDDHISM_MONK).r_pkills * 10;

    return result;
}

bool AllianceBinzyouBuddhism::isAnnihilated()
{
    return all_monraces_extinct({ MonraceId::BINZYOU_MUR });
}
