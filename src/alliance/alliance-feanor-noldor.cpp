#include "alliance/alliance-feanor-noldor.h"
#include "alliance/alliance.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief フェアノール統ノルドールアライアンスの印象ポイント計算
 * @param creature_ptr プレイヤー情報
 * @return 印象ポイント
 */
int AllianceFeanorNoldor::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 19, 26);
    return impression;
}

/*!
 * @brief フェアノール統ノルドールアライアンスの制裁処理
 * @param player_ptr プレイヤー情報
 * @details 現在は空実装
 */
void AllianceFeanorNoldor::panishment([[maybe_unused]] PlayerType &player_ptr)
{
    // TODO: フェアノール統ノルドールの制裁システムを実装
    msg_print("フェアノール統ノルドールの制裁が発動した！");
}

/*!
 * @brief フェアノール統ノルドールアライアンスの壊滅判定
 * @return 壊滅しているかどうか
 * @details 現在は空実装
 */
bool AllianceFeanorNoldor::isAnnihilated()
{
    // TODO: フェアノール統ノルドールの壊滅条件を実装
    return false;
}
