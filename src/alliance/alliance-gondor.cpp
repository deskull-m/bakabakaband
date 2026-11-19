#include "alliance/alliance-gondor.h"
#include "alliance/alliance.h"
#include "system/creature-entity.h"
#include "view/display-messages.h"

/*!
 * @brief ゴンドールアライアンスの印象ポイント計算
 * @param creature クリーチャーへの参照
 * @return 印象ポイント
 * @details 現在は空実装
 */
int AllianceGondor::calcImpressionPoint(const CreatureEntity &creature) const
{
    int impression = 0;
    impression += calcIronmanHostilityPenalty();

    // トールキンの善玉陣営: 善のアライメントほどそこそこプラス、悪の場合は大きめにマイナス
    impression += (creature.alignment > 0) ? creature.alignment : creature.alignment * 3;

    return impression;
}

/*!
 * @brief ゴンドールアライアンスの制裁処理
 * @param creature クリーチャーへの参照
 * @details 現在は空実装
 */
void AllianceGondor::panishment([[maybe_unused]] CreatureEntity &creature)
{
    // TODO: ゴンドールの制裁システムを実装
    // msg_print("ゴンドールの制裁が発動した！");
}

/*!
 * @brief ゴンドールアライアンスの壊滅判定
 * @return 壊滅しているかどうか
 * @details 現在は空実装
 */
bool AllianceGondor::isAnnihilated()
{
    // TODO: ゴンドールの壊滅条件を実装
    return false;
}
