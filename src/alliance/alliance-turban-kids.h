#pragma once
#include "alliance/alliance.h"

class AllianceTurbanKids : public Alliance {
public:
    using Alliance::Alliance;
    AllianceTurbanKids() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(const CreatureEntity &creature) const override;
    void panishment(CreatureEntity &creature) override;
    virtual ~AllianceTurbanKids() = default;
};
