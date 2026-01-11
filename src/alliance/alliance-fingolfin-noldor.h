#pragma once

#include "alliance/alliance.h"

class AllianceFingolfinNoldor : public Alliance {
public:
    using Alliance::Alliance;
    AllianceFingolfinNoldor() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(const CreatureEntity &creature) const override;
    void panishment(CreatureEntity &creature) override;
    bool isAnnihilated() override;
    virtual ~AllianceFingolfinNoldor() = default;
};
