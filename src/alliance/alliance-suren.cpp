#include "alliance/alliance-suren.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"

int AllianceSuren::calcImpressionPoint([[maybe_unused]] const CreatureEntity &creature) const
{
    int result = 0;
    result += calcIronmanHostilityPenalty();

    const auto &monraces = MonraceList::get_instance();

    // スレン王 (レベル30)
    result -= monraces.get_monrace(MonraceId::SUREN).r_pkills * 300;

    // スレン王の兵卒 (レベル6)
    result -= monraces.get_monrace(MonraceId::PAWN_OF_KING_SUREN).r_pkills * 60;

    // スレン王の重兵卒 (レベル20)
    result -= monraces.get_monrace(MonraceId::GREAT_PAWN_OF_KING_SUREN).r_pkills * 200;

    // スレン王の騎士 (レベル19)
    result -= monraces.get_monrace(MonraceId::KNIGHT_OF_KING_SUREN).r_pkills * 190;

    // スレン王の重騎士 (レベル24)
    result -= monraces.get_monrace(MonraceId::GREAT_KNIGHT_OF_KING_SUREN).r_pkills * 240;

    return result;
}

bool AllianceSuren::isAnnihilated()
{
    return all_monraces_extinct({ MonraceId::SUREN });
}
