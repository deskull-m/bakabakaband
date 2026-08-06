#pragma once

#include "alliance/alliance.h"

class AllianceArioch : public Alliance {
public:
    using Alliance::Alliance;
    AllianceArioch() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(const CreatureEntity &creature) const override;
    virtual ~AllianceArioch() = default;
};
