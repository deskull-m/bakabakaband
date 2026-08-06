#pragma once

#include "alliance/alliance.h"

class AllianceAshinaClan : public Alliance {
public:
    using Alliance::Alliance;
    AllianceAshinaClan() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(const CreatureEntity &creature) const override;
    virtual ~AllianceAshinaClan() = default;
};
