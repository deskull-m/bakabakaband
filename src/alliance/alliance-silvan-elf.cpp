#include "alliance/alliance-silvan-elf.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"

#include "game-option/birth-options.h"
int AllianceSilvanElf::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    // 鉄人モード: 全てのアライアンスから猛烈に敵対される
    if (ironman_alliance_hostility) {
        impression -= 10000;
    }

    impression += Alliance::calcPlayerPower(*creature_ptr, 12, 25);

    // シルヴァンエルフの指導者を殺害した場合の大幅減点
    const auto &monrace_list = MonraceList::get_instance();
    if (monrace_list.get_monrace(MonraceId::THRANDUIL).r_pkills > 0) {
        impression -= 2000; // 森エルフの王『スランドゥイル』を殺害
    }
    if (monrace_list.get_monrace(MonraceId::LEGOLAS).r_pkills > 0) {
        impression -= 1500; // 緑葉の『レゴラス』を殺害
    }

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
    // 森エルフの王『スランドゥイル』と緑葉の『レゴラス』が両方とも存在しない場合、シルヴァンエルフは壊滅する
    const auto &monrace_list = MonraceList::get_instance();
    return monrace_list.get_monrace(MonraceId::THRANDUIL).mob_num == 0 && monrace_list.get_monrace(MonraceId::LEGOLAS).mob_num == 0;
}
