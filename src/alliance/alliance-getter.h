#pragma once

#include "alliance/alliance.h"

class AllianceGetter : public Alliance {
public:
    using Alliance::Alliance;
    AllianceGetter() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    virtual ~AllianceGetter() = default;
};
