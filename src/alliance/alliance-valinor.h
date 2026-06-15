#pragma once
#include "alliance/alliance.h"

class AllianceValinor : public Alliance {
public:
    using Alliance::Alliance;
    AllianceValinor() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(const CreatureEntity &creature) const override;
    virtual ~AllianceValinor() = default;
};
