#pragma once

#include "alliance/alliance.h"

class AllianceAngartha : public Alliance {
public:
    using Alliance::Alliance;
    AllianceAngartha() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    virtual ~AllianceAngartha() = default;
};
