#pragma once

#include "alliance/alliance.h"

class AllianceKhaine : public Alliance {
public:
    using Alliance::Alliance;
    AllianceKhaine() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(const CreatureEntity &creature) const override;
    void panishment(CreatureEntity &creature) override;
    bool isAnnihilated() override;
};
