#pragma once

#include "alliance/alliance.h"

class AllianceNone : public Alliance {
public:
    using Alliance::Alliance;
    AllianceNone() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const override;
    virtual ~AllianceNone() = default;
};
