#include "alliance/alliance-amber.h"
#include "monster-race/race-kind-flags.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"

int AllianceAmber::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 10, 35);
    impression += calcIronmanHostilityPenalty();

    // アンバライト・アライアンス関連モンスターの撃破による印象値減少（掛け算）
    const auto &monrace_list = MonraceList::get_instance();

    // 撃破数に基づく減少倍率計算
    int total_kills = 0;

    // 主要アンバライト君主の撃破数（重み付き）
    total_kills += monrace_list.get_monrace(MonraceId::OBERON).r_akills * 500; // アンバーの王『オベロン』
    total_kills += monrace_list.get_monrace(MonraceId::DWORKIN).r_akills * 400; // 線の巨匠『ドワーキン』
    total_kills += monrace_list.get_monrace(MonraceId::BENEDICT).r_akills * 350; // 理想の戦士『ベネディクト』
    total_kills += monrace_list.get_monrace(MonraceId::BRAND).r_akills * 300; // 狂気の夢想家『ブランド』
    total_kills += monrace_list.get_monrace(MonraceId::CORWIN).r_akills * 250; // アヴァロンの主『コーウィン』
    total_kills += monrace_list.get_monrace(MonraceId::BLEYS).r_akills * 250; // ごまかしの名手『ブレイズ』
    total_kills += monrace_list.get_monrace(MonraceId::ERIC).r_akills * 250; // 王位簒奪者『エリック』
    total_kills += monrace_list.get_monrace(MonraceId::CAINE).r_akills * 200; // 陰謀家『ケイン』
    total_kills += monrace_list.get_monrace(MonraceId::JULIAN).r_akills * 200; // アーデンの森の主『ジュリアン』
    total_kills += monrace_list.get_monrace(MonraceId::GERARD).r_akills * 200; // アンバーの強者『ジェラード』
    total_kills += monrace_list.get_monrace(MonraceId::FIONA).r_akills * 200; // 女魔術師『フィオナ』
    total_kills += monrace_list.get_monrace(MonraceId::RINALDO).r_akills * 150; // ブランドの息子『リナルド』

    // 掛け算による減少：撃破数が多いほど印象値が大幅に減少
    if (total_kills > 0) {
        double reduction_factor = 1.0 - (total_kills * 0.01); // 1%ずつ減少
        if (reduction_factor < 0.1) {
            reduction_factor = 0.1; // 最低10%は残す
        }
        impression = static_cast<int>(impression * reduction_factor);
    }

    return impression;
}

/*!
 * @brief アンバーのアライアンスが壊滅したかどうか判定する
 * @return 壊滅フラグ
 * @details アンバーの王『オベロン』が存在しない場合に壊滅する
 */
bool AllianceAmber::isAnnihilated()
{
    // アンバーの王『オベロン』が存在しない場合、アンバーは壊滅する
    // return MonraceList::get_instance().get_monrace(MonraceId::OBERON).mob_num == 0;
    return false;
}

/*!
 * @brief 襲撃時に出現するモンスターのリストを取得する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param impression_point 印象値
 * @return アンバーのモンスターIDのリスト（印象値が低い場合は天使系や騎士系モンスター）
 */
std::vector<MonraceId> AllianceAmber::get_ambush_monsters([[maybe_unused]] PlayerType *player_ptr, int impression_point) const
{
    std::vector<MonraceId> monsters;

    // 印象値が低い場合のみ襲撃を行う（-200以下）
    if (impression_point >= -200) {
        return monsters;
    }

    // 印象値に応じてモンスターを追加（アンバーは秩序側なので天使や騎士系）
    monsters.push_back(MonraceId::KACHO_ANGEL); // 課長天使
    monsters.push_back(MonraceId::FALLEN_ANGEL); // 堕天使
    monsters.push_back(MonraceId::ULTRA_PALADIN); // ウルトラパラディン

    // 印象値が非常に低い場合は強力なユニークも追加
    if (impression_point < -500) {
        monsters.push_back(MonraceId::JULIAN); // アーデンの森の主『ジュリアン』
        monsters.push_back(MonraceId::CAINE); // 陰謀家『ケイン』
    }

    return monsters;
}

/*!
 * @brief 襲撃時のメッセージを取得する
 * @return アンバー固有の襲撃メッセージ
 */
std::string AllianceAmber::get_ambush_message() const
{
    return _("アンバーの使者があなたを阻む！", "The agents of Amber block your way!");
}
