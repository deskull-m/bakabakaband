#pragma once
#include "alliance/alliance.h"

class AllianceKetholdeth : public Alliance {
public:
    using Alliance::Alliance;
    AllianceKetholdeth() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    bool isAnnihilated() override;
    virtual ~AllianceKetholdeth() = default;
};
