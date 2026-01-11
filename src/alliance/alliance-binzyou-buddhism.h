#pragma once

#include "alliance/alliance.h"

class AllianceBinzyouBuddhism : public Alliance {
public:
    using Alliance::Alliance;
    AllianceBinzyouBuddhism() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(const CreatureEntity &creature) const override;
    bool isAnnihilated() override;
    virtual ~AllianceBinzyouBuddhism() = default;
};
