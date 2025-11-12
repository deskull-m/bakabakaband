#include "alliance/alliance-aryan-family.h"
#include "alliance/alliance.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"

int AllianceAryanFamily::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 14, 22);

    // アーリアン・ファミリーのアライアンス所属モンスター撃破による印象値減少
    const std::string aryan_family_tag = get_alliance_type_tag(AllianceType::ARYAN_FAMILY);
    const std::string alliance_kill_key = "KILL/ALLIANCE/" + aryan_family_tag;
    if (creature_ptr->incident_tree.count(alliance_kill_key)) {
        impression -= creature_ptr->incident_tree.at(alliance_kill_key) * 15; // 1体につき-15
    }

    // 個別モンスター撃破による追加印象値減少
    const auto &monrace_list = MonraceList::get_instance();

    // ボス『ビッグ・アイ』撃破による大幅減点
    if (monrace_list.get_monrace(MonraceId::BIG_EYE).r_pkills > 0) {
        impression -= 500; // ボス撃破による大幅ペナルティ
    }

    // 悲しみのレイベン撃破による減点
    impression -= monrace_list.get_monrace(MonraceId::RAVEN_THE_GRIEF).r_pkills * 200;

    // コモドドラゴン撃破による減点
    impression -= monrace_list.get_monrace(MonraceId::KOMODO_DRAGON).r_pkills * 10;

    // アーリアン・ファミリーの構成員撃破による減点
    impression -= monrace_list.get_monrace(MonraceId::ARYAN_FAMILY_MEMBER).r_pkills * 5;

    return impression;
}

/*!
 * @brief アーリアンファミリーのアライアンスが壊滅したかどうか判定する
 * @return 壊滅フラグ
 * @details アーリアン・ファミリーのボス『ビッグ・アイ』が存在しない場合に壊滅する
 */
bool AllianceAryanFamily::isAnnihilated()
{
    // アーリアン・ファミリーのボス『ビッグ・アイ』が存在しない場合、アーリアンファミリーは壊滅する
    return MonraceList::get_instance().get_monrace(MonraceId::BIG_EYE).mob_num == 0;
}
