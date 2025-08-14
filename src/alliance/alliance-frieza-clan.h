#pragma once

#include "alliance/alliance.h"

class AllianceFriezaClan : public Alliance {
public:
    using Alliance::Alliance;
    AllianceFriezaClan() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    void panishment(PlayerType &player_ptr) override;
    bool isAnnihilated() override;
    virtual ~AllianceFriezaClan() = default;
};
