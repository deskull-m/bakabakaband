#include "alliance/alliance-feanor-noldor.h"
#include "alliance/alliance.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "view/display-messages.h"

/*!
 * @brief フェアノール統ノルドールアライアンスの印象ポイント計算
 * @param creature クリーチャーへの参照
 * @return 印象ポイント
 */
int AllianceFeanorNoldor::calcImpressionPoint(const CreatureEntity &creature) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(creature, 19, 26);
    impression += calcIronmanHostilityPenalty();

    return impression;
}

/*!
 * @brief フェアノール統ノルドールアライアンスの制裁処理
 * @param creature クリーチャーへの参照
 * @details 現在は空実装
 */
void AllianceFeanorNoldor::panishment([[maybe_unused]] CreatureEntity &creature)
{
    // TODO: フェアノール統ノルドールの制裁システムを実装
    // msg_print("フェアノール統ノルドールの制裁が発動した！");
}

/*!
 * @brief フェアノール統ノルドールアライアンスの壊滅判定
 * @return 壊滅しているかどうか
 * @details 憤怒の上級王『フェアノール』が存在しない場合に壊滅する
 */
bool AllianceFeanorNoldor::isAnnihilated()
{
    // 憤怒の上級王『フェアノール』が存在しない場合、フェアノール統ノルドールは壊滅する
    return MonraceList::get_instance().get_monrace(MonraceId::FEANOR).mob_num == 0;
}
