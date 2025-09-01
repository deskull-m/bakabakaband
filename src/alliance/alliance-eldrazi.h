#pragma once
#include "alliance/alliance.h"

class AllianceEldrazi : public Alliance {
public:
    using Alliance::Alliance;
    AllianceEldrazi() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    virtual ~AllianceEldrazi() = default;
};
