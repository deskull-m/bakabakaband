#pragma once

#include "alliance/alliance.h"

class AllianceMeldor : public Alliance {
public:
    using Alliance::Alliance;
    AllianceMeldor() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    virtual ~AllianceMeldor() = default;
};
