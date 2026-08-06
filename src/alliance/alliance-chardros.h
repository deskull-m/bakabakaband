#pragma once

#include "alliance/alliance.h"

class AllianceChardros : public Alliance {
public:
    using Alliance::Alliance;
    AllianceChardros() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(const CreatureEntity &creature) const override;
    virtual ~AllianceChardros() = default;
};
