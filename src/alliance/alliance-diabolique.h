#pragma once

#include "alliance/alliance.h"

class AllianceDiabolique : public Alliance {
public:
    using Alliance::Alliance;
    AllianceDiabolique() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    void panishment(PlayerType &player_ptr) override;
    bool isAnnihilated() override;
    virtual ~AllianceDiabolique() = default;
};