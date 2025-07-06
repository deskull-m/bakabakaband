#pragma once

#include "alliance/alliance.h"

class AllianceFangFamily : public Alliance {
public:
    using Alliance::Alliance;
    AllianceFangFamily() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    void panishment(PlayerType &player_ptr) override;
    bool isAnnihilated() override;
    virtual ~AllianceFangFamily() = default;
};
