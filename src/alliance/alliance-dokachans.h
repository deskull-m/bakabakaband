#pragma once

#include "alliance/alliance.h"

class AllianceDokachans : public Alliance {
public:
    using Alliance::Alliance;
    AllianceDokachans() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    bool isAnnihilated() override;
    virtual ~AllianceDokachans() = default;
};
