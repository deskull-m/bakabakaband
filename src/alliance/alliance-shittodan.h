#pragma once

#include "alliance/alliance.h"

class AllianceShittoDan : public Alliance {
public:
    using Alliance::Alliance;
    AllianceShittoDan() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(const CreatureEntity &creature) const override;
    void panishment(CreatureEntity &creature) override;
    bool isAnnihilated() override;
    virtual ~AllianceShittoDan() = default;
};