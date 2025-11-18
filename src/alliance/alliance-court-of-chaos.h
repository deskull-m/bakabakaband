#pragma once

#include "alliance/alliance.h"

class PlayerType;

class AllianceCourtOfChaos : public Alliance {
public:
    using Alliance::Alliance;
    AllianceCourtOfChaos() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    virtual ~AllianceCourtOfChaos() = default;
};
