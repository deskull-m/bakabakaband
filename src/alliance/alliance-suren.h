#pragma once

#include "alliance/alliance.h"

class AllianceSuren : public Alliance {
public:
    using Alliance::Alliance;
    AllianceSuren() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    bool isAnnihilated() override;
    virtual ~AllianceSuren() = default;
};
