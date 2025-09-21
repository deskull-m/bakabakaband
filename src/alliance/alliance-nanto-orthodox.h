/*!
 * @file alliance-nanto-orthodox.h
 * @brief アライアンス「南斗正統派」の定義
 */
#pragma once

#include "alliance/alliance.h"

class PlayerType;

/*!
 * @brief アライアンス「南斗正統派」のクラス
 */
class AllianceNantoOrthodox : public Alliance {
public:
    AllianceNantoOrthodox(AllianceType id, std::string tag, std::string name, int64_t base_power);
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    bool isAnnihilated() override;
};