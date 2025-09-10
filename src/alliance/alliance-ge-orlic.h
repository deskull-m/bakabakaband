#pragma once

#include "alliance/alliance.h"

class AllianceGEOrlic : public Alliance {
public:
    using Alliance::Alliance;
    AllianceGEOrlic() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    virtual ~AllianceGEOrlic() = default;
};
