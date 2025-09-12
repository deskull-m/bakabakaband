#pragma once

#include "alliance/alliance.h"

class AllianceGaichi : public Alliance {
public:
    using Alliance::Alliance;
    AllianceGaichi() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    bool isAnnihilated() override;
    virtual ~AllianceGaichi() = default;
};
