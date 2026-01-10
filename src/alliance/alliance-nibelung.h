#pragma once

#include "alliance/alliance.h"

class AllianceNibelung : public Alliance {
public:
    using Alliance::Alliance;
    AllianceNibelung() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    void panishment(CreatureEntity &creature) override;
    bool isAnnihilated() override;
    virtual ~AllianceNibelung() = default;
};
