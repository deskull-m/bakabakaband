#pragma once

#include "alliance.h"

class AllianceAnorLondo : public Alliance {
public:
    using Alliance::Alliance;
    AllianceAnorLondo() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    void panishment(CreatureEntity &creature) override;
    bool isAnnihilated() override;
};
