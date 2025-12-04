#include "alliance/alliance-incubetor.h"
#include "alliance/alliance.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

#include "game-option/birth-options.h"
/*!
 * @brief インキュベーターアライアンスの印象ポイント計算
 * @param creature_ptr プレイヤー情報
 * @return 印象ポイント
 * @details 魔法少女や契約に関連した特殊な評価基準
 */
int AllianceIncubetor::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    // 鉄人モード: 全てのアライアンスから猛烈に敵対される
    if (ironman_alliance_hostility) {
        impression -= 10000;
    }
    // 魔法少女的な存在として、魔力や知力を重視
    impression += Alliance::calcPlayerPower(*creature_ptr, 12, 15);
    return impression;
}

/*!
 * @brief インキュベーターアライアンスの制裁処理
 * @param player_ptr プレイヤー情報
 * @details 契約違反に対する制裁
 */
void AllianceIncubetor::panishment([[maybe_unused]] PlayerType &player_ptr)
{
    // TODO: インキュベーターの制裁システムを実装
    // msg_print("契約に基づく制裁が発動した！");
}

/*!
 * @brief インキュベーターアライアンスの壊滅判定
 * @return 壊滅しているかどうか
 * @details 現在は空実装
 */
bool AllianceIncubetor::isAnnihilated()
{
    // TODO: インキュベーターの壊滅条件を実装
    return false;
}
