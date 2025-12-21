#include "alliance/alliance-frieza-clan.h"
#include "alliance/alliance.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief フリーザ一族アライアンスの印象ポイント計算
 * @param creature_ptr プレイヤー情報
 * @return 印象ポイント
 * @details 宇宙の帝王として高い戦闘力を求める
 */
int AllianceFriezaClan::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    // 宇宙の帝王として、高レベルの強者を重視
    impression += Alliance::calcPlayerPower(*creature_ptr, 25, 30);
    impression += calcIronmanHostilityPenalty();

    return impression;
}

/*!
 * @brief フリーザ一族アライアンスの制裁処理
 * @param player_ptr プレイヤー情報
 * @details 裏切り者に対する厳しい制裁
 */
void AllianceFriezaClan::panishment([[maybe_unused]] PlayerType &player_ptr)
{
    // TODO: フリーザ一族の制裁システムを実装
    // msg_print("宇宙の帝王の怒りが降り注ぐ！");
}

/*!
 * @brief フリーザ一族アライアンスの壊滅判定
 * @return 壊滅しているかどうか
 * @details 現在は空実装
 */
bool AllianceFriezaClan::isAnnihilated()
{
    // TODO: フリーザ一族の壊滅条件を実装
    return false;
}
