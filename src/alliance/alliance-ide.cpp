#include "alliance/alliance-ide.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"

/*!
 * @brief イデのアライアンス印象値を計算する
 * 無限力の存在として、INTとWISをベースとした印象値計算を行う
 * @param creature クリーチャーへの参照
 * @return 印象値
 */
int AllianceIde::calcImpressionPoint(const CreatureEntity &creature) const
{
    // bias = 20, level = 35 (イデは無限力の存在なので、非常に高い基準を持つ)
    int bias = 20;
    int level = 35;

    int base_stat = (creature.get_stat_max(A_INT) + creature.get_stat_max(A_WIS)) / 2;
    int impression = base_stat + bias + creature.get_level() / level;

    // 最低値保証
    if (impression < 1) {
        impression = 1;
    }

    return impression;
}

/*!
 * @brief イデのアライアンス懲罰処理
 * @param creature クリーチャーへの参照
 */
void AllianceIde::panishment([[maybe_unused]] CreatureEntity &creature)
{
    // 基本的な懲罰処理を実装
}

/*!
 * @brief イデのアライアンスが壊滅したかどうか判定する
 * @return 壊滅フラグ
 */
bool AllianceIde::isAnnihilated()
{
    return all_monraces_extinct({ MonraceId::IDE });
}
