#pragma once

#include "alliance/alliance.h"

class AllianceLegendOfSavior : public Alliance {
public:
    using Alliance::Alliance;
    AllianceLegendOfSavior() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    bool isAnnihilated() override;
    virtual ~AllianceLegendOfSavior() = default;
    void panishment(PlayerType &player_ptr) override;
};