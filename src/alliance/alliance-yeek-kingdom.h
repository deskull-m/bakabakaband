#pragma once
#include "alliance/alliance.h"

class AllianceYeekKingdom : public Alliance {
public:
    using Alliance::Alliance;
    AllianceYeekKingdom() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(const CreatureEntity &creature) const override;
    bool isAnnihilated() override;
    void panishment(CreatureEntity &creature) override;
    virtual ~AllianceYeekKingdom() = default;
};
