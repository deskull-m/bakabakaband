#include "alliance/alliance-eagle-clan.h"
#include "alliance/alliance.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"

/*!
 * @brief 大鷲の一族の印象値を計算する
 * @param creature クリーチャーへの参照
 * @return 印象値
 * @details マンウェに遣わされたマイアールの魂を持つ大鷲達は善なる者を好む。
 * 善の傾向が強いほど印象が良くなり、悪の傾向は強く嫌われる。
 * 一族の大鷲を殺害している場合は大幅な減点を受ける。
 */
int AllianceEagleClan::calcImpressionPoint(const CreatureEntity &creature) const
{
    int impression = 0;
    impression += calcIronmanHostilityPenalty();

    impression += (creature.alignment > 0) ? creature.alignment : creature.alignment * 3;

    // 一族の大鷲を殺害した場合の減点
    const auto &monrace_list = MonraceList::get_instance();
    if (monrace_list.get_monrace(MonraceId::THORONDOR).r_pkills > 0) {
        impression -= 3000; // 大鷲王『ソロンドール』を殺害
    }
    if (monrace_list.get_monrace(MonraceId::GWAIHIR).r_pkills > 0) {
        impression -= 1500; // 風の支配者『グワイヒア』を殺害
    }
    if (monrace_list.get_monrace(MonraceId::MENELDOR).r_pkills > 0) {
        impression -= 1000; // 疾翼『メネルドール』を殺害
    }

    return impression;
}

/*!
 * @brief 大鷲の一族の制裁処理
 * @param creature クリーチャーへの参照
 */
void AllianceEagleClan::panishment(CreatureEntity &creature)
{
    auto impression = calcImpressionPoint(creature);
    if (isAnnihilated() || impression > -40) {
        return;
    }
}

/*!
 * @brief 大鷲の一族が壊滅しているかを判定する
 * @return 筆頭たる『ソロンドール』を含む主要な大鷲が全て絶滅していればtrue
 */
bool AllianceEagleClan::isAnnihilated()
{
    // 筆頭『ソロンドール』とグワイヒア・メネルドールが全て存在しなくなった場合、大鷲の一族は壊滅する
    return all_monraces_extinct({ MonraceId::THORONDOR, MonraceId::GWAIHIR, MonraceId::MENELDOR });
}
