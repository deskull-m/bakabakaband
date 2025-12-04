#include "alliance/alliance-ungoliant.h"
#include "alliance/alliance.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"

#include "game-option/birth-options.h"
AllianceUngoliant::AllianceUngoliant(AllianceType id, std::string tag, std::string name, int64_t base_power)
    : Alliance(id, tag, name, base_power)
{
}

int AllianceUngoliant::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    // 鉄人モード: 全てのアライアンスから猛烈に敵対される
    if (ironman_alliance_hostility) {
        impression -= 10000;
    }

    impression += Alliance::calcPlayerPower(*creature_ptr, 8, 30);

    // ウンゴリアント一族の指導者を殺害した場合の大幅減点
    const auto &monrace_list = MonraceList::get_instance();
    if (monrace_list.get_monrace(MonraceId::UNGOLIANT).r_pkills > 0) {
        impression -= 2500; // 光なき闇の大蜘蛛『ウンゴリアント』を殺害
    }
    if (monrace_list.get_monrace(MonraceId::SHELOB).r_pkills > 0) {
        impression -= 1800; // 闇の蜘蛛『シェロブ』を殺害
    }

    return impression;
}

bool AllianceUngoliant::isAnnihilated()
{
    // 光なき闇の大蜘蛛『ウンゴリアント』と闇の蜘蛛『シェロブ』が両方とも存在しない場合、ウンゴリアント一族は壊滅する
    const auto &monrace_list = MonraceList::get_instance();
    return monrace_list.get_monrace(MonraceId::UNGOLIANT).mob_num == 0 && monrace_list.get_monrace(MonraceId::SHELOB).mob_num == 0;
}
