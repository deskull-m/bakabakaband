#pragma once
#include "alliance.h"

class AllianceTurbanKids : public Alliance {
public:
    using Alliance::Alliance;
    AllianceTurbanKids() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    void panishment(PlayerType &player_ptr) override;
    virtual ~AllianceTurbanKids() = default;
};
