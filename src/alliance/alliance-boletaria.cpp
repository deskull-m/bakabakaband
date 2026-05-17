#include "alliance/alliance-boletaria.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"

/*!
 * @brief ボーレタリアのアライアンス印象値を計算する
 * 騎士の王国として、STRとCONをベースとした印象値計算を行う
 * @param creature クリーチャーへの参照
 * @return 印象値
 */
int AllianceBoletaria::calcImpressionPoint(const CreatureEntity &creature) const
{
    int bias = 10;
    int level = 22;

    int base_stat = (creature.get_stat_max(A_STR) + creature.get_stat_max(A_CON)) / 2;
    int impression = base_stat + bias + creature.get_level() / level;

    if (impression < 1) {
        impression = 1;
    }

    return impression;
}

/*!
 * @brief ボーレタリアのアライアンス懲罰処理
 * @param creature クリーチャーへの参照
 */
void AllianceBoletaria::panishment([[maybe_unused]] CreatureEntity &creature)
{
    // 基本的な懲罰処理を実装
}

/*!
 * @brief ボーレタリアのアライアンスが壊滅したかどうか判定する
 * @return 壊滅フラグ
 * @details 老王『オーラント』が存在しない場合に壊滅する
 */
bool AllianceBoletaria::isAnnihilated()
{
    // 老王『オーラント』が存在しない場合、ボーレタリアは壊滅する
    return MonraceList::get_instance().get_monrace(MonraceId::OLD_KING_ALLANT).mob_num == 0;
}
