#pragma once

#include "alliance/alliance.h"

class AllianceKING : public Alliance {
public:
    using Alliance::Alliance;
    AllianceKING() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    virtual ~AllianceKING() = default;
};
