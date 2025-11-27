#include "alliance/alliance-fingolfin-noldor.h"
#include "alliance/alliance.h"
#include "game-option/birth-options.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief フィンゴルフィン統ノルドールアライアンスの印象ポイント計算
 * @param creature_ptr プレイヤー情報
 * @return 印象ポイント
 * @details フェアノール統より若干穏健な評価基準
 */
int AllianceFingolfinNoldor::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 17, 24);
    // 鉄人モード: 全てのアライアンスから猛烈に敵対される
    if (ironman_alliance_hostility) {
        impression -= 10000;
    }

    return impression;
}

/*!
 * @brief フィンゴルフィン統ノルドールアライアンスの制裁処理
 * @param player_ptr プレイヤー情報
 * @details 現在は空実装
 */
void AllianceFingolfinNoldor::panishment([[maybe_unused]] PlayerType &player_ptr)
{
    // TODO: フィンゴルフィン統ノルドールの制裁システムを実装
    // msg_print("フィンゴルフィン統ノルドールの制裁が発動した！");
}

/*!
 * @brief フィンゴルフィン統ノルドールアライアンスの壊滅判定
 * @return 壊滅しているかどうか
 * @details 聡明の上級王『フィンゴルフィン』が存在しない場合に壊滅する
 */
bool AllianceFingolfinNoldor::isAnnihilated()
{
    // 聡明の上級王『フィンゴルフィン』が存在しない場合、フィンゴルフィン統ノルドールは壊滅する
    return MonraceList::get_instance().get_monrace(MonraceId::FINGOLFIN).mob_num == 0;
}
