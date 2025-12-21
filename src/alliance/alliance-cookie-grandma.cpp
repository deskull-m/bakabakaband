#include "alliance/alliance-cookie-grandma.h"
#include "alliance/alliance.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief クッキーババアアライアンスの印象ポイント計算
 * @param creature_ptr プレイヤー情報
 * @return 印象ポイント
 * @details 現在は空実装
 */
int AllianceCookieGrandma::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += calcIronmanHostilityPenalty();

    // TODO: クッキーババアの価値観に基づく印象ポイント計算を実装
    return impression;
}

/*!
 * @brief クッキーババアアライアンスの制裁処理
 * @param player_ptr プレイヤー情報
 * @details 現在は空実装
 */
void AllianceCookieGrandma::panishment([[maybe_unused]] PlayerType &player_ptr)
{
    // TODO: クッキーババアの制裁システムを実装
    // msg_print("クッキーババアの制裁が発動した！");
}

/*!
 * @brief クッキーババアアライアンスの壊滅判定
 * @return 壊滅しているかどうか
 * @details 現在は空実装
 */
bool AllianceCookieGrandma::isAnnihilated()
{
    // TODO: クッキーババアの壊滅条件を実装
    return false;
}
