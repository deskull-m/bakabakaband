#include "alliance/alliance-valverde.h"
#include "alliance/alliance.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief バルベルデ共和国アライアンスの印象ポイント計算
 * @param creature_ptr プレイヤー情報
 * @return 印象ポイント
 * @details 現在は空実装
 */
int AllianceValVerde::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += calcIronmanHostilityPenalty();

    // TODO: バルベルデ共和国の価値観に基づく印象ポイント計算を実装
    return impression;
}

/*!
 * @brief バルベルデ共和国アライアンスの制裁処理
 * @param player_ptr プレイヤー情報
 * @details 現在は空実装
 */
void AllianceValVerde::panishment([[maybe_unused]] PlayerType &player_ptr)
{
    // TODO: バルベルデ共和国の制裁システムを実装
    // msg_print("バルベルデ共和国の制裁が発動した！");
}

/*!
 * @brief バルベルデ共和国アライアンスの壊滅判定
 * @return 壊滅しているかどうか
 * @details 現在は空実装
 */
bool AllianceValVerde::isAnnihilated()
{
    // TODO: バルベルデ共和国の壊滅条件を実装
    return false;
}
