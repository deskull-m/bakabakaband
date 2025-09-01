#pragma once

#include "alliance/alliance.h"

class PlayerType;

class AllianceKoganRyu : public Alliance {
public:
    using Alliance::Alliance;
    AllianceKoganRyu() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    virtual ~AllianceKoganRyu() = default;
};
