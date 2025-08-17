#include "alliance/alliance-silvan-elf.h"
#include "system/player-type-definition.h"

int AllianceSilvanElf::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 12, 25);
    return impression;
}

void AllianceSilvanElf::panishment(PlayerType &player_ptr)
{
    auto impression = calcImpressionPoint(&player_ptr);
    if (isAnnihilated() || impression > -40) {
        return;
    }
}

bool AllianceSilvanElf::isAnnihilated()
{
    // TODO: 適切なモンスターIDに置き換える
    return false; // MonraceList::get_instance().get_monrace(MonraceId::HAFU_SHOGUN).mob_num == 0;
}
