#include "alliance/alliance-anor-londo.h"
#include "game-option/birth-options.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
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
    int impression = base_stat + bias + creature_ptr->level / level;
    // 鉄人モード: 全てのアライアンスから猛烈に敵対される
    if (ironman_alliance_hostility) {
        impression -= 10000;
    }

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
 * @return 壊滅フラグ - 太陽の光の王グウィンが死亡した場合true
 */
bool AllianceAnorLondo::isAnnihilated()
{
    const auto &monrace_list = MonraceList::get_instance();
    return monrace_list.get_monrace(MonraceId::GWYN).mob_num == 0;
}
