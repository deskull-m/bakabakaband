#include "alliance/alliance-ashina-clan.h"
#include "game-option/birth-options.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"

int AllianceAshinaClan::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 5, 20);
    // 鉄人モード: 全てのアライアンスから猛烈に敵対される
    if (ironman_alliance_hostility) {
        impression -= 10000;
    }

    // 芦名一門のモンスター撃破による印象値低下
    const auto &monrace_list = MonraceList::get_instance();

    // ユニークモンスターの撃破による印象値減少（レベル×10）
    if (monrace_list.get_monrace(MonraceId::GENICHIRO_ASHINA).r_pkills > 0) {
        impression -= 290; // 巴流『葦名弦一郎』を殺害 (レベル29 × 10)
    }
    if (monrace_list.get_monrace(MonraceId::NAOMORI_KAWARADA).r_pkills > 0) {
        impression -= 230; // 侍大将『川原田直盛』を殺害 (レベル23 × 10)
    }
    if (monrace_list.get_monrace(MonraceId::SHIGENORI_YAMAUCHI).r_pkills > 0) {
        impression -= 190; // 組頭『山内重則』を殺害 (レベル19 × 10)
    }

    // 一般モンスターの撃破による印象値減少（複数撃破可能なため撃破数に応じて減点）
    impression -= monrace_list.get_monrace(MonraceId::ASHINA_ASHIGARU).r_pkills * 8; // 葦名の足軽 (レベル8 × 1)
    impression -= monrace_list.get_monrace(MonraceId::ASHINA_RYU_DISCIPLE).r_pkills * 17; // 葦名流門弟 (レベル17 × 1)
    impression -= monrace_list.get_monrace(MonraceId::TARO_SOLDIER).r_pkills * 26; // 太郎兵 (レベル26 × 1)
    impression -= monrace_list.get_monrace(MonraceId::CHAINED_OGRE_ASHINA).r_pkills * 31; // 葦名の赤鬼 (レベル31 × 1)

    // 芦名一門全体の撃破数による追加ペナルティ
    const std::string alliance_kill_key = "KILL/ALLIANCE/ASHINA-CLAN";
    if (creature_ptr->incident_tree.count(alliance_kill_key)) {
        impression -= creature_ptr->incident_tree.at(alliance_kill_key) * 12; // 1体につき-12
    }

    return impression;
}
