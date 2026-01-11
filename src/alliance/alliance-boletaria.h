#pragma once

#include "alliance.h"

class AllianceBoletaria : public Alliance {
public:
    using Alliance::Alliance;
    AllianceBoletaria() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(const CreatureEntity &creature) const override;
    void panishment(CreatureEntity &creature) override;
    bool isAnnihilated() override;
};
