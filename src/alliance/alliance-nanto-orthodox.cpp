/*!
 * @file alliance-nanto-orthodox.cpp
 * @brief アライアンス「南斗正統派」の実装
 */
#include "alliance/alliance-nanto-orthodox.h"
#include "floor/floor-util.h"
#include "floor/wild.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
/*!
 * @brief アライアンス「南斗正統派」のコンストラクタ
 * @param id アライアンスID
 * @param tag アライアンスタグ
 * @param name アライアンス名
 * @param base_power 基本戦力
 */
AllianceNantoOrthodox::AllianceNantoOrthodox(AllianceType id, std::string tag, std::string name, int64_t base_power)
    : Alliance(id, tag, name, base_power)
{
}

/*!
 * @brief 南斗正統派におけるプレイヤーの印象度を算出する
 * @param creature_ptr プレイヤーへの参照ポインタ
 * @return 印象度
 */
int AllianceNantoOrthodox::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 15, 20);
    return impression;
}

/*!
 * @brief 南斗正統派が全滅したか判定する
 * @return 全滅していればtrue
 */
bool AllianceNantoOrthodox::isAnnihilated()
{
    return false;
}
