#include "alliance/alliance-triothepunch.h"
#include "alliance/alliance.h"
#include "effect/effect-characteristics.h"
#include "floor/floor-util.h"
#include "monster-floor/monster-summon.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

int AllianceTrioThePunch::calcImpressionPoint(PlayerType *creature_ptr) const
{
    auto impression = Alliance::calcPlayerPower(*creature_ptr, 10, 5);

    // トリオ・ザ・パンチのメンバーを殺害した場合の減点
    const auto &monrace_list = MonraceList::get_instance();
    if (monrace_list.get_monrace(MonraceId::KAMAKURAKUN).r_pkills > 0) {
        impression -= 390; // 忍者『カマクラくん』を殺害 (レベル39 × 10)
    }
    if (monrace_list.get_monrace(MonraceId::SANTOS).r_pkills > 0) {
        impression -= 380; // タフガイ『サントス』を殺害 (レベル38 × 10)
    }
    if (monrace_list.get_monrace(MonraceId::ROSESABU).r_pkills > 0) {
        impression -= 390; // 剣士『ロースサブ』を殺害 (レベル39 × 10)
    }
    if (monrace_list.get_monrace(MonraceId::MASTER_CHEN).r_pkills > 0) {
        impression -= 490; // 師匠『チンさん』を殺害 (レベル49 × 10)
    }

    return impression;
}

bool AllianceTrioThePunch::isAnnihilated()
{
    // 全てのメンバーが倒されている場合、トリオ・ザ・パンチは壊滅する
    const auto &monrace_list = MonraceList::get_instance();
    return monrace_list.get_monrace(MonraceId::KAMAKURAKUN).mob_num == 0 && monrace_list.get_monrace(MonraceId::SANTOS).mob_num == 0 && monrace_list.get_monrace(MonraceId::ROSESABU).mob_num == 0 && monrace_list.get_monrace(MonraceId::MASTER_CHEN).mob_num == 0;
}
