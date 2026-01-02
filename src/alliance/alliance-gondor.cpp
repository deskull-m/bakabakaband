#include "alliance/alliance-gondor.h"
#include "alliance/alliance.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief ゴンドールアライアンスの印象ポイント計算
 * @param creature_ptr プレイヤー情報
 * @return 印象ポイント
 * @details 現在は空実装
 */
int AllianceGondor::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += calcIronmanHostilityPenalty();

    // TODO: ゴンドールの価値観に基づく印象ポイント計算を実装
    return impression;
}

/*!
 * @brief ゴンドールアライアンスの制裁処理
 * @param player_ptr プレイヤー情報
 * @details 現在は空実装
 */
void AllianceGondor::panishment([[maybe_unused]] PlayerType &player_ptr)
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
