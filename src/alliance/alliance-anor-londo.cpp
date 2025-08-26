#include "alliance/alliance-anor-londo.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "system/player-type-definition.h"

/*!
 * @brief アノール・ロンドのアライアンス印象値を計算する
 * 太陽と光の都市として、INTとCHRをベースとした印象値計算を行う
 * @param creature_ptr プレイヤーへの参照ポインタ
 * @return 印象値
 */
int AllianceAnorLondo::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int bias = 12;
    int level = 25;

    int base_stat = (creature_ptr->stat_max[A_INT] + creature_ptr->stat_max[A_CHR]) / 2;
    int impression = base_stat + bias + creature_ptr->lev / level;

    // 最低値保証
    if (impression < 1) {
        impression = 1;
    }

    return impression;
}

/*!
 * @brief アノール・ロンドのアライアンス懲罰処理
 * @param player_ptr プレイヤーへの参照
 */
void AllianceAnorLondo::panishment([[maybe_unused]] PlayerType &player_ptr)
{
    // 基本的な懲罰処理を実装
}

/*!
 * @brief アノール・ロンドのアライアンスが壊滅したかどうか判定する
 * @return 壊滅フラグ
 */
bool AllianceAnorLondo::isAnnihilated()
{
    return false;
}
