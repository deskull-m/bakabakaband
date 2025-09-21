#include "alliance/alliance-ide.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"

/*!
 * @brief イデのアライアンス印象値を計算する
 * 無限力の存在として、INTとWISをベースとした印象値計算を行う
 * @param creature_ptr プレイヤーへの参照ポインタ
 * @return 印象値
 */
int AllianceIde::calcImpressionPoint(PlayerType *creature_ptr) const
{
    // bias = 20, level = 35 (イデは無限力の存在なので、非常に高い基準を持つ)
    int bias = 20;
    int level = 35;

    // INTとWISの平均をベースとした印象値計算（無限力らしく知性と叡智を重視）
    int base_stat = (creature_ptr->stat_max[A_INT] + creature_ptr->stat_max[A_WIS]) / 2;
    int impression = base_stat + bias + creature_ptr->lev / level;

    // 最低値保証
    if (impression < 1) {
        impression = 1;
    }

    return impression;
}

/*!
 * @brief イデのアライアンス懲罰処理
 * @param player_ptr プレイヤーへの参照
 */
void AllianceIde::panishment([[maybe_unused]] PlayerType &player_ptr)
{
    // 基本的な懲罰処理を実装
}

/*!
 * @brief イデのアライアンスが壊滅したかどうか判定する
 * @return 壊滅フラグ
 */
bool AllianceIde::isAnnihilated()
{
    return MonraceList::get_instance().get_monrace(MonraceId::IDE).mob_num == 0;
}
