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
    impression += calcIronmanHostilityPenalty();

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

/*!
 * @brief 襲撃時に出現するモンスターのリストを取得する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param impression_point 印象値
 * @return 混沌の宮廷のモンスターIDのリスト（印象値が低い場合はカオス系モンスター）
 */
std::vector<MonraceId> AllianceCourtOfChaos::get_ambush_monsters([[maybe_unused]] PlayerType *player_ptr, int impression_point) const
{
    std::vector<MonraceId> monsters;

    // 印象値が低い場合のみ襲撃を行う（-150以下）
    if (impression_point >= -150) {
        return monsters;
    }

    // 印象値に応じてモンスターを追加（混沌側なのでカオス系クリーチャー）
    monsters.push_back(MonraceId::LORD_CHAOS); // 混沌の王族
    monsters.push_back(MonraceId::BLUE_HORROR); // ブルーホラー
    monsters.push_back(MonraceId::PINK_HORROR); // ピンクホラー
    monsters.push_back(MonraceId::BLOODLETTER); // ブラッドレター

    // 印象値が非常に低い場合は混沌の宮廷のユニークも追加
    if (impression_point < -400) {
        monsters.push_back(MonraceId::JURT); // 人間トランプ『ジャート』
        monsters.push_back(MonraceId::MANDOR); // ログルスの達人『マンドール』
    }

    return monsters;
}

/*!
 * @brief 襲撃時のメッセージを取得する
 * @return 混沌の宮廷固有の襲撃メッセージ
 */
std::string AllianceCourtOfChaos::get_ambush_message() const
{
    return _("混沌の宮廷の尖兵が現れた！", "The vanguard of the Court of Chaos appears!");
}
