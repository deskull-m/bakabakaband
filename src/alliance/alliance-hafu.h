#pragma once

#include "alliance/alliance.h"

class AllianceHafu : public Alliance {
public:
    using Alliance::Alliance;
    AllianceHafu() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(const CreatureEntity &creature) const override;
    void panishment(CreatureEntity &creature) override;
    bool isAnnihilated() override;
    virtual ~AllianceHafu() = default;
};