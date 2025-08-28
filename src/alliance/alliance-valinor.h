#pragma once
#include "alliance.h"

class AllianceValinor : public Alliance {
public:
    using Alliance::Alliance;
    AllianceValinor() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    virtual ~AllianceValinor() = default;
};
