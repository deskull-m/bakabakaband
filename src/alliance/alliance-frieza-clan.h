#pragma once

#include "alliance/alliance.h"

class AllianceFriezaClan : public Alliance {
public:
    using Alliance::Alliance;
    AllianceFriezaClan() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(const CreatureEntity &creature) const override;
    void panishment(CreatureEntity &creature) override;
    bool isAnnihilated() override;
    virtual ~AllianceFriezaClan() = default;
};
