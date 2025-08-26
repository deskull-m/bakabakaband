#include "alliance/alliance-boletaria.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "system/player-type-definition.h"

/*!
 * @brief ボーレタリアのアライアンス印象値を計算する
 * 騎士の王国として、STRとCONをベースとした印象値計算を行う
 * @param creature_ptr プレイヤーへの参照ポインタ
 * @return 印象値
 */
int AllianceBoletaria::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int bias = 10;
    int level = 22;

    int base_stat = (creature_ptr->stat_max[A_STR] + creature_ptr->stat_max[A_CON]) / 2;
    int impression = base_stat + bias + creature_ptr->lev / level;

    if (impression < 1) {
        impression = 1;
    }

    return impression;
}

/*!
 * @brief ボーレタリアのアライアンス懲罰処理
 * @param player_ptr プレイヤーへの参照
 */
void AllianceBoletaria::panishment([[maybe_unused]] PlayerType &player_ptr)
{
    // 基本的な懲罰処理を実装
}

/*!
 * @brief ボーレタリアのアライアンスが壊滅したかどうか判定する
 * @return 壊滅フラグ
 */
bool AllianceBoletaria::isAnnihilated()
{
    return false;
}
