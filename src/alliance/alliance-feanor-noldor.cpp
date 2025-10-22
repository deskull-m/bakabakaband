#include "alliance/alliance-feanor-noldor.h"
#include "alliance/alliance.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
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
