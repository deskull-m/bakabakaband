#include "alliance/alliance-court-of-chaos.h"
#include "alliance/alliance.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"

int AllianceCourtOfChaos::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 10, 35);

    // 混沌の宮廷のメンバーを殺害した場合の減点
    const auto &monrace_list = MonraceList::get_instance();

    // ユニークモンスター
    if (monrace_list.get_monrace(MonraceId::JURT).r_pkills > 0) {
        impression -= 340; // 人間トランプ『ジャート』を殺害 (レベル34 × 10)
    }
    if (monrace_list.get_monrace(MonraceId::LORD_BOREL).r_pkills > 0) {
        impression -= 1000; // 『ボレル公爵』を殺害 (原作中の特別視により大幅減点)
    }
    if (monrace_list.get_monrace(MonraceId::MANDOR).r_pkills > 0) {
        impression -= 380; // ログルスの達人『マンドール』を殺害 (レベル38 × 10)
    }
    if (monrace_list.get_monrace(MonraceId::JASRA).r_pkills > 0) {
        impression -= 400; // ブランドの情婦『ジャスラ』を殺害 (レベル40 × 10)
    }
    if (monrace_list.get_monrace(MonraceId::STRYGALLDWIR).r_pkills > 0) {
        impression -= 410; // 『ストリガルドワー』を殺害 (レベル41 × 10)
    }
    if (monrace_list.get_monrace(MonraceId::RINALDO).r_pkills > 0) {
        impression -= 410; // ブランドの息子『リナルド』を殺害 (レベル41 × 10)
    }

    // 一般モンスター（複数撃破の可能性があるため、撃破数に応じて減点）
    impression -= monrace_list.get_monrace(MonraceId::LORD_CHAOS).r_pkills * 60; // 混沌の王族を殺害 (レベル60 × 1 per kill)

    return impression;
}
