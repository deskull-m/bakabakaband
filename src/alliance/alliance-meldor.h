#pragma once

#include "alliance/alliance.h"

class AllianceMeldor : public Alliance {
public:
    using Alliance::Alliance;
    AllianceMeldor() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    bool isAnnihilated() override; // 修正: const を削除
    virtual ~AllianceMeldor() = default;
};
