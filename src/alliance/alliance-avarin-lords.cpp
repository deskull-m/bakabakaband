#include "alliance/alliance-avarin-lords.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"

int AllianceAvarinLords::calcImpressionPoint(const CreatureEntity &creature) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(creature, 10, 23);
    impression += calcIronmanHostilityPenalty();

    // アヴァリ諸侯同盟のモンスター撃破による印象値低下
    const auto &monrace_list = MonraceList::get_instance();

    // ユニークモンスターの撃破による印象値減少（レベル×10）
    if (monrace_list.get_monrace(MonraceId::EOL_DARK_ELF_SMITH).r_pkills > 0) {
        impression -= 400; // ダークエルフの鍛冶師『エオル』を殺害 (レベル40 × 10)
    }
    if (monrace_list.get_monrace(MonraceId::MAEGLIN_BETRAYER).r_pkills > 0) {
        impression -= 560; // ゴンドリンを裏切りし『マイグリン』を殺害 (レベル56 × 10)
    }

    // 一般モンスターの撃破による印象値減少（複数撃破可能なため撃破数に応じて減点）
    impression -= monrace_list.get_monrace(MonraceId::AVARIN_RANGER).r_pkills * 42; // アヴァリ族のレンジャー (レベル42 × 1)

    // アヴァリ諸侯同盟全体の撃破数による追加ペナルティ
    const std::string alliance_kill_key = "KILL/ALLIANCE/AVARIN-LORDS";
    if (creature.incident_tree.count(alliance_kill_key)) {
        impression -= creature.incident_tree.at(alliance_kill_key) * 15; // 1体につき-15
    }

    return impression;
}
