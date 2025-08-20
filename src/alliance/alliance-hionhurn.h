#pragma once

#include "alliance/alliance.h"

class AllianceHionhurn : public Alliance {
public:
    using Alliance::Alliance;
    AllianceHionhurn() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    virtual ~AllianceHionhurn() = default;
};
