#pragma once

#include "alliance/alliance.h"

class AllianceHide : public Alliance {
public:
    using Alliance::Alliance;
    AllianceHide() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    void panishment(CreatureEntity &creature) override;
    bool isAnnihilated() override;
    virtual ~AllianceHide() = default;
};
