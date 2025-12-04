#pragma once

#include "alliance/alliance.h"

class AllianceBasamEmpire : public Alliance {
public:
    using Alliance::Alliance;
    AllianceBasamEmpire() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    bool isAnnihilated() override;
    virtual ~AllianceBasamEmpire() = default;
};
