/*!
 * @file alliance-kenohgun.h
 * @brief アライアンス「拳王軍」の定義
 */
#pragma once

#include "alliance/alliance.h"

class PlayerType;

/*!
 * @brief アライアンス「拳王軍」のクラス
 */
class AllianceKenohgun : public Alliance {
public:
    AllianceKenohgun(AllianceType id, std::string tag, std::string name, int64_t base_power);
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    bool isAnnihilated() override;
};
