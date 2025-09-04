#pragma once

#include "alliance/alliance.h"

class PlayerType;

class AllianceNumenor : public Alliance {
public:
    using Alliance::Alliance;
    AllianceNumenor() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    virtual ~AllianceNumenor() = default;
};
